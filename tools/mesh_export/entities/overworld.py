import mathutils
import io
import struct
import time
from . import mesh
from . import bounding_box
from . import mesh_split
from . import tiny3d_mesh_writer
from . import mesh_collider
from . import export_settings

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
    def __init__(self, mesh_data: bytes, y_offset: float, y_scale: float):
        self.mesh_data: bytes = mesh_data
        self.y_offset: float = y_offset
        self.y_scale: float = y_scale

    def __str__(self):
        return f"mesh_data(len) {len(self.mesh_data)} y_offset {self.y_offset} y_scale {self.y_scale}"
    
LOD_0_SCALE = 1 / 128

def generate_overworld(overworld_filename: str, mesh_list: mesh.mesh_list, lod_0_mesh: mesh.mesh_list, collider: mesh_collider.MeshCollider, subdivisions: int, settings: export_settings.ExportSettings):
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
    
    for y, row in enumerate(cells):
        for x, cell in enumerate(row):
            cell_bb = cell[0].bounding_box()

            for i in range(1, len(cell)):
                cell_bb = bounding_box.union(cell_bb, cell[i].bounding_box())

            height = cell_bb[1].y - cell_bb[0].y
            y_scale = max_block_height / height if height > max_block_height else 1

            for mesh_data in cell:
                mesh_data.translate(mathutils.Vector((
                    -(x * side_length + mesh_bb[0].x), 
                    -cell_bb[0].y, 
                    -(y * side_length + mesh_bb[0].z)
                )))
                mesh_data.scale(mathutils.Vector((
                    1, 
                    y_scale, 
                    1
                )))

            write_mesh_time -= time.perf_counter()
            data = io.BytesIO()
            tiny3d_mesh_writer.write_mesh(cell, None, [], settings, data)
            data.write(struct.pack('>ff', cell_bb[0].y, y_scale))
            cell_data.append(OverworldCell(data.getvalue(), cell_bb[0].y, y_scale))
            write_mesh_time += time.perf_counter()

            write_collider_time -= time.perf_counter()
            collider_data = io.BytesIO()
            collider_cells[y][x][0].write_out(collider_data)
            collider_cell_data.append(collider_data.getvalue())
            write_collider_time += time.perf_counter()

    print(f"write_mesh_time = {write_mesh_time}")
    print(f"write_collider_time = {write_collider_time}")

    lod_0_mesh_bytes = io.BytesIO()
    lod_0_settings = settings.copy()
    lod_0_settings.sort_direction = mathutils.Vector((1, 0, 0))
    lod_0_mesh_data = lod_0_mesh.determine_mesh_data(None)
    for entry in lod_0_mesh_data:
        entry.scale(LOD_0_SCALE)
    tiny3d_mesh_writer.write_mesh(lod_0_mesh_data, None, [], lod_0_settings, lod_0_mesh_bytes)

    with open(overworld_filename, 'wb') as file:
        file.write('OVWD'.encode())

        file.write(struct.pack('>HH', subdivisions, subdivisions))
        file.write(struct.pack('>ff', mesh_bb[0].x, mesh_bb[0].z))
        file.write(struct.pack('>f', side_length))

        for i in range(4):
            file.write(lod_0_mesh_bytes.getvalue())

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