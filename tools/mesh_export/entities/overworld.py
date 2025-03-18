import mathutils
import bpy
import io
import struct
import time
import math
from . import mesh
from . import bounding_box
from . import mesh_split
from . import tiny3d_mesh_writer
from . import mesh_collider
from . import export_settings
from . import material_extract

def subdivide_mesh_list(meshes: list[mesh.mesh_data], normal: mathutils.Vector, start_pos: float, distance_step: float, subdivisions: int) -> list[list[mesh.mesh_data]]:
    result = []
    distance = -(distance_step + start_pos)

    for i in range(subdivisions - 1):
        chunk = []
        next = []

        for mesh in meshes:
            behind, front = mesh_split.split(mesh, normal, distance)

            if behind:
                chunk.append(behind)

            if front:
                next.append(front)
        
        meshes = next
        distance -= distance_step
        result.append(chunk)

    result.append(meshes)

    return result
    
max_block_height = 32767 / 64

class OverworldCell():
    def __init__(self, mesh_data: bytes, y_offset: float):
        self.mesh_data: bytes = mesh_data
        self.y_offset: float = y_offset

    def __str__(self):
        return f"mesh_data(len) {len(self.mesh_data)} y_offset {self.y_offset}"

def generate_overworld_tile(cell: list[mesh.mesh_data], side_length: float, x: int, z: int, map_min: mathutils.Vector, settings: export_settings.ExportSettings):
    cell_bb = cell[0].bounding_box()

    for i in range(1, len(cell)):
        cell_bb = bounding_box.union(cell_bb, cell[i].bounding_box())

    height = cell_bb[1].y - cell_bb[0].y

    y_cells = subdivide_mesh_list(
        cell, 
        mathutils.Vector((0, 1, 0)), 
        cell_bb[0].y, 
        side_length,
        math.ceil(height / side_length)
    )

    data = io.BytesIO()
    data.write(struct.pack('>B', len(y_cells)))
    data.write(struct.pack('>f', cell_bb[0].y))

    for y, y_cell in enumerate(y_cells):
        for mesh_data in y_cell:
            mesh_data.translate(mathutils.Vector((
                -(x * side_length + map_min.x), 
                -(y * side_length + cell_bb[0].y), 
                -(z * side_length + map_min.z)
            )))
            
        tiny3d_mesh_writer.write_mesh(y_cell, None, [], settings, data)

    return OverworldCell(data.getvalue(), cell_bb[0].y)

def generate_lod0(lod_0_objects: list[bpy.types.Object], subdivisions: int, settings: export_settings.ExportSettings, base_transform: mathutils.Matrix, file):
    lod_0_start_time = time.perf_counter()

    lod_0_settings = settings.copy()
    lod_0_settings.sort_direction = mathutils.Vector((1, 0, 0))
    lod_0_settings.fog_scale = 1 / subdivisions

    scaled_transform = mathutils.Matrix.Scale(1 / subdivisions, 4) @ base_transform

    all_meshes: list[tuple[mesh.mesh_data, int, int, int]] = []

    for obj in lod_0_objects:
        mesh_list = mesh.mesh_list(scaled_transform)
        mesh_list.append(obj)

        center = scaled_transform @ obj.matrix_world.translation

        digit_prefix_length = 0

        while digit_prefix_length < len(obj.name) and obj.name[digit_prefix_length].isdigit():
            digit_prefix_length += 1

        priority = int(obj.name[0: digit_prefix_length]) if digit_prefix_length > 0 else 0

        all_meshes += map(lambda mesh: (mesh, int(center.x), int(center.z), priority), mesh_list.determine_mesh_data(None))

    file.write(struct.pack('>B', len(all_meshes)))

    for single_mesh in all_meshes:
        file.write(struct.pack('>hhH', single_mesh[1], single_mesh[2], single_mesh[3]))
        lod_0_settings.default_material_name = material_extract.material_romname(single_mesh[0].mat)
        lod_0_settings.default_material = material_extract.load_material_with_name(single_mesh[0].mat)
        tiny3d_mesh_writer.write_mesh([single_mesh[0]], None, [], lod_0_settings, file)

    print(f"lod_0 creation time {time.perf_counter() - lod_0_start_time}")

def generate_overworld(overworld_filename: str, mesh_list: mesh.mesh_list, lod_0_objects: list[bpy.types.Object], collider: mesh_collider.MeshCollider, subdivisions: int, settings: export_settings.ExportSettings, base_transform: mathutils.Matrix):
    mesh_entries = mesh_list.determine_mesh_data()

    mesh_bb = None

    for entry in mesh_entries:
        entry_bb = entry.bounding_box()

        if mesh_bb:
            mesh_bb = bounding_box.union(mesh_bb, entry_bb)
        else:
            mesh_bb = entry_bb

    width = mesh_bb[1].x - mesh_bb[0].x
    height = mesh_bb[1].z - mesh_bb[0].z

    side_length = max(width, height) / subdivisions

    subdivide_time_start = time.perf_counter()
    columns = subdivide_mesh_list(mesh_entries, mathutils.Vector((0, 0, 1)), mesh_bb[0].x, side_length, subdivisions)
    cells = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].z, side_length, subdivisions), columns))
    print(f"subdivide_mesh_list for mesh data {time.perf_counter() - subdivide_time_start}")

    subivide_collider_start = time.perf_counter()
    collider_columns = subdivide_mesh_list([collider], mathutils.Vector((0, 0, 1)), mesh_bb[0].x, side_length, subdivisions)
    collider_cells: list[list[list[mesh_collider.MeshCollider]]] = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].z, side_length, subdivisions), collider_columns))
    print(f"subdivide_mesh_list for collider data {time.perf_counter() - subivide_collider_start}")

    cell_data: list[OverworldCell] = []
    collider_cell_data: list[bytes] = []

    write_mesh_time = 0
    write_collider_time = 0
    
    for z, row in enumerate(cells):
        for x, cell in enumerate(row):
            write_mesh_time -= time.perf_counter()
            cell_data.append(generate_overworld_tile(
                cell,
                side_length,
                x,
                z,
                mesh_bb[0],
                settings
            ))
            write_mesh_time += time.perf_counter()

            write_collider_time -= time.perf_counter()
            collider_data = io.BytesIO()
            if len(collider_cells[z][x]):
                collider_cells[z][x][0].write_out(collider_data, force_subdivisions = mathutils.Vector((8, 1, 8)))
            else:
                tmp = mesh_collider.MeshCollider()
                tmp.write_out(collider_data, force_subdivisions=mathutils.Vector((8, 1, 8)))
            collider_cell_data.append(collider_data.getvalue())
            write_collider_time += time.perf_counter()

    print(f"write_mesh_time = {write_mesh_time}")
    print(f"write_collider_time = {write_collider_time}")

    with open(overworld_filename, 'wb') as file:
        file.write('OVWD'.encode())

        file.write(struct.pack('>HH', subdivisions, subdivisions))
        file.write(struct.pack('>ff', mesh_bb[0].x, mesh_bb[0].z))
        file.write(struct.pack('>f', side_length))

        generate_lod0(lod_0_objects, subdivisions, settings, base_transform, file)

        visual_block_location = file.tell() + 8 * subdivisions * subdivisions

        actor_block_location = visual_block_location + sum(map(lambda x: len(x.mesh_data), cell_data))

        for index, cell in enumerate(cell_data):
            file.write(struct.pack('>II', visual_block_location, actor_block_location))
            visual_block_location += len(cell.mesh_data)
            actor_block_location += len(collider_cell_data[index])

        for cell in cell_data:
            file.write(cell.mesh_data)

        for actor_cell in collider_cell_data:
            file.write(actor_cell)