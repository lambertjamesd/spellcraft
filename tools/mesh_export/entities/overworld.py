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
from . import filename
from . import entities
from . import particles

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
    
coordinate_convert = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')
coordinate_convert_invert = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

class OverworldDetail():
    def __init__(self, obj: bpy.types.Object):
        self.obj: bpy.types.Object = obj
        self.final_transform: mathutils.Matrix = coordinate_convert_invert @ obj.matrix_world @ coordinate_convert
        loc, rot, scale = self.final_transform.decompose()
        self.position: mathutils.Vector = loc
        self.mesh_filename = filename.rom_filename(obj.data.library.filepath) + '.tmesh'
        

def subdivide_detail_list(detail_list: list, normal: mathutils.Vector, start_pos: float, distance_step: float, subdivisions: int) -> list[list]:
    result: list[list] = []

    for i in range(subdivisions):
        result.append([])

    for detail in detail_list:
        bin_index = math.floor((normal.dot(detail.position) - start_pos) / distance_step)
        bin_index = max(bin_index, 0)
        bin_index = min(bin_index, subdivisions - 1)

        result[bin_index].append(detail)

    return result

def generate_overworld_tile(
        cell: list[mesh.mesh_data], 
        details: list[OverworldDetail], 
        particle_list: list[particles.Particles], 
        side_length: float, 
        x: int, z: int, 
        map_min: mathutils.Vector, 
        settings: export_settings.ExportSettings):
    if len(cell) == 0:
        data = io.BytesIO()
        data.write(struct.pack('>BHH', 0, 0, 0))
        data.write(struct.pack('>f', 0))
        particles.write_particles(particle_list, data)
        return OverworldCell(data.getvalue(), 0)


    cell_bb = cell[0].bounding_box()

    for i in range(1, len(cell)):
        cell_bb = bounding_box.union(cell_bb, cell[i].bounding_box())

    mesh_names = list(set(map(lambda x: x.mesh_filename, details)))

    height = cell_bb[1].y - cell_bb[0].y

    vertical_subdivisions = math.ceil(height / side_length)

    y_cells = subdivide_mesh_list(
        cell, 
        mathutils.Vector((0, 1, 0)), 
        cell_bb[0].y, 
        side_length,
        vertical_subdivisions
    )

    detail_cells = subdivide_detail_list(details, mathutils.Vector((0, 1, 0)), cell_bb[0].y, side_length, vertical_subdivisions)
    data = io.BytesIO()
    data.write(struct.pack('>BHH', len(y_cells), len(mesh_names), len(details)))
    data.write(struct.pack('>f', cell_bb[0].y))

    for name in mesh_names:
        name_bytes = name.encode()
        data.write(struct.pack('>B', len(name_bytes)))
        data.write(name_bytes)

    for y, y_cell in enumerate(y_cells):
        cell_corner = mathutils.Vector((
            -(x * side_length + map_min.x), 
            -(y * side_length + cell_bb[0].y), 
            -(z * side_length + map_min.z)
        ))

        for mesh_data in y_cell:
            mesh_data.translate(cell_corner)

        scrolling_meshes: list[mesh.mesh_data] = []
        static_meshes: list[mesh.mesh_data] = []

        for mesh_data in y_cell:
            mat = material_extract.load_material_with_name(mesh_data.mat)
            if mat.does_scroll():
                scrolling_meshes.append(mesh_data)
            else:
                static_meshes.append(mesh_data)
            
        tiny3d_mesh_writer.write_mesh(static_meshes, None, [], settings, data)

        cell_details = detail_cells[y] if y < len(detail_cells) else []
        data.write(struct.pack('>H', len(cell_details)))

        for detail in cell_details:
            data.write(struct.pack('>H', mesh_names.index(detail.mesh_filename)))
            pos, rot, scale = detail.final_transform.decompose()
            adjusted_pos = (pos + cell_corner) * settings.fixed_point_scale

            scale = scale * 0.5

            data.write(struct.pack('>fff', adjusted_pos.x, adjusted_pos.y, adjusted_pos.z))
            data.write(struct.pack('>ffff', rot.x, rot.y, rot.z, rot.w))
            data.write(struct.pack('>fff', scale.x, scale.y, scale.z))

        scrolling_meshes = sorted(scrolling_meshes, key = lambda x: x.mat_priority())

        data.write(len(scrolling_meshes).to_bytes(2, 'big'))

        pre_sort_count = 0

        for mesh in scrolling_meshes:
            if mesh.mat_priority() < 10:
                pre_sort_count += 1

        data.write(pre_sort_count.to_bytes(2, 'big'))

        for mesh_data in scrolling_meshes:
            settings_copy = settings.copy()
            settings_copy.default_material_name = material_extract.material_romname(mesh_data.mat)
            settings_copy.default_material = material_extract.load_material_with_name(mesh_data.mat)

            tiny3d_mesh_writer.write_mesh([mesh_data], None, [], settings_copy, data)


    particle_list = sorted(particle_list, key = lambda x: x.material.name)

    particles.write_particles(particle_list, data)

    return OverworldCell(data.getvalue(), cell_bb[0].y)

sort_directions = [
    mathutils.Vector((1, 0, 0)),
    mathutils.Vector((-1, 0, 0)),
    mathutils.Vector((0, 0, 1)),
    mathutils.Vector((0, 0, -1)),
]

SKYBOX_RENDER_OFFSET = 10

def obj_bounding_box(obj: bpy.types.Object) -> tuple[mathutils.Vector, mathutils.Vector]:
    final_transform = obj.matrix_world
    transformed = [final_transform @ mathutils.Vector(vtx) for vtx in obj.bound_box]

    bb_min = transformed[0]
    bb_max = transformed[0]

    for vtx in transformed:
        bb_min = mathutils.Vector((
            min(bb_min.x, vtx.x),
            min(bb_min.y, vtx.y),
            min(bb_min.z, vtx.z)
        ))
        bb_max = mathutils.Vector((
            max(bb_max.x, vtx.x),
            max(bb_max.y, vtx.y),
            max(bb_max.z, vtx.z)
        ))

    return bb_min, bb_max

def bounding_box_intersection(a: tuple[mathutils.Vector, mathutils.Vector], b: tuple[mathutils.Vector, mathutils.Vector]) -> tuple[mathutils.Vector, mathutils.Vector]:
    bb_min = mathutils.Vector((
        max(a[0].x, b[0].x),
        max(a[0].y, b[0].y),
        max(a[0].z, b[0].z)
    ))
    bb_max = mathutils.Vector((
        min(a[1].x, b[1].x),
        min(a[1].y, b[1].y),
        min(a[1].z, b[1].z)
    ))

    return bb_min, mathutils.Vector((
        max(bb_min.x, bb_max.x),
        max(bb_min.y, bb_max.y),
        max(bb_min.z, bb_max.z)
    ))

def bounding_box_volume(a: tuple[mathutils.Vector, mathutils.Vector]):
    return (a[1].x - a[0].x) * (a[1].y - a[0].y) * (a[1].z - a[0].z)

class LodTile():
    def __init__(self, obj: bpy.types.Object, level: int):
        self.obj: bpy.types.Object = obj
        self.level: int = level
        self.children: list = []

        bb_min, bb_max = obj_bounding_box(self.obj)

        if bb_min.z + 1 > bb_max.z:
            bb_max.z = bb_min.z + 1

        self.bb: tuple[mathutils.Vector, mathutils.Vector] = (bb_min, bb_max)

        digit_prefix_length = 0

        while digit_prefix_length < len(self.obj.name) and (self.obj.name[digit_prefix_length].isdigit() or (digit_prefix_length == 0 and self.obj.name[0] == '-')):
            digit_prefix_length += 1

        self.sort_prefix = int(self.obj.name[0: digit_prefix_length]) if digit_prefix_length > 0 else 0

    def priority(self):
        result = -self.sort_prefix

        if not self.is_skybox():
            result += self.level * SKYBOX_RENDER_OFFSET
        else:
            result += 100

        return result
    
    def is_skybox(self):
        return 'skybox' in self.obj.name
    
    def child_count(self) -> int:
        result = len(self.children)

        for child in self.children:
            result += child.child_count()

        return result
    
    def does_contain(self, child) -> bool:
        if self.sort_prefix != child.sort_prefix:
            return False

        intersection = bounding_box_intersection(self.bb, child.bb)

        return bounding_box_volume(intersection) > bounding_box_volume(child.bb) * 0.5
    
    def flatten_into(self, result: list):
        result.append(self)
        for child in self.children:
            child.flatten_into(result)

    def debug_print(self, indent = ''):
        print(indent + self.obj.name + ' pri=' + str(self.priority()))

        for child in self.children:
            child.debug_print('    ' + indent)
    
def separate_lods(lod_1_objects: list[LodTile]) -> list[list[LodTile]]:
    result: list[list[LodTile]] = []

    for obj in lod_1_objects:
        index = obj.level

        if obj.is_skybox():
            index = 0

        while len(result) <= index:
            result.append([])

        result[index].append(obj)

    return result

def map_children(child_lod: list[LodTile], parent_lod: list[LodTile]) -> list[LodTile]:
    unused: list[LodTile] = []
    
    for child in child_lod:
        use_parent = None
        for parent in parent_lod:
            if parent.does_contain(child):
                use_parent = parent
                break

        if use_parent:
            use_parent.children.append(child)
        else:
            unused.append(child)

    return unused

def generate_lod0(lod_1_objects: list[LodTile], subdivisions: int, settings: export_settings.ExportSettings, base_transform: mathutils.Matrix, file):
    lod_1_start_time = time.perf_counter()

    lod_1_settings = settings.copy()
    lod_1_settings.sort_direction = mathutils.Vector((1, 0, 0))
    lod_1_settings.fog_scale = 1 / subdivisions

    scaled_transform = mathutils.Matrix.Scale(1 / subdivisions, 4) @ base_transform
    center_scale = settings.world_scale * 0.5

    root_objects = []

    grouped_objects: list[list[LodTile]] = separate_lods(lod_1_objects)

    if len(grouped_objects) > 0:
        root_objects += grouped_objects[0]
    if len(grouped_objects) > 1:
        root_objects += grouped_objects[-1]


    for i in range(1, len(grouped_objects) - 1):
        root_objects += map_children(grouped_objects[i], grouped_objects[i + 1])

    for root in root_objects:
        root.debug_print()

    ordered_objects: list[LodTile] = []

    for root in root_objects:
        root.flatten_into(ordered_objects)

    file.write(struct.pack('>B', len(ordered_objects)))

    for tile in ordered_objects:
        mesh_list = mesh.mesh_list(scaled_transform)
        mesh_list.append(tile.obj)

        center = (scaled_transform @ tile.obj.matrix_world.translation) * center_scale

        priority = tile.priority()

        mesh_data = mesh_list.determine_mesh_data(None)

        file.write(struct.pack('>hhHBB', int(center.x), int(center.z), priority, tile.child_count(), 2 ** (tile.level - 1)))

        mesh_bytes_file = io.BytesIO()

        if len(mesh_data) > 0:
            lod_1_settings.default_material_name = material_extract.material_romname(mesh_data[0].mat)
            lod_1_settings.default_material = material_extract.load_material_with_name(mesh_data[0].mat)

        for dir in sort_directions:
            lod_1_settings.sort_direction = dir
            tiny3d_mesh_writer.write_mesh(mesh_data, None, [], lod_1_settings, mesh_bytes_file)

        mesh_bytes = mesh_bytes_file.getvalue()

        file.write(len(mesh_bytes).to_bytes(4, 'big'))
        file.write(mesh_bytes)

    print(f"lod_1 creation time {time.perf_counter() - lod_1_start_time}")

class OverworldInputData():
    def __init__(self):
        pass

def generate_overworld(
        overworld_filename: str, 
        mesh_list: mesh.mesh_list, 
        lod_1_objects: list[LodTile], 
        collider: mesh_collider.MeshCollider, 
        detail_list: list[OverworldDetail], 
        entity_list: list[entities.ObjectEntry],
        particles_list: list[particles.Particles], 
        subdivisions: int, 
        settings: export_settings.ExportSettings, 
        base_transform: mathutils.Matrix,
        enums: dict,
        variable_context
        ):
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

    print('side_length', side_length)

    for detail in detail_list:
        if 'collider' in detail.obj.data:
            collider.append(detail.obj.data['collider'], coordinate_convert_invert @ detail.obj.matrix_world)

    subdivide_time_start = time.perf_counter()
    columns = subdivide_mesh_list(mesh_entries, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    cells = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), columns))
    print(f"subdivide_mesh_list for mesh data {time.perf_counter() - subdivide_time_start}")

    subivide_collider_start = time.perf_counter()
    collider_columns = subdivide_mesh_list([collider], mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    collider_cells: list[list[list[mesh_collider.MeshCollider]]] = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), collider_columns))
    print(f"subdivide_mesh_list for collider data {time.perf_counter() - subivide_collider_start}")

    detail_columns = subdivide_detail_list(detail_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    detail_cells: list[list[list[OverworldDetail]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), detail_columns))

    entity_columns = subdivide_detail_list(entity_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    entity_cells: list[list[list[entities.ObjectEntry]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), entity_columns))

    particle_columns = subdivide_detail_list(particles_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    particle_cells: list[list[list[particles.Particles]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), particle_columns))

    cell_data: list[OverworldCell] = []
    collider_cell_data: list[bytes] = []

    write_mesh_time = 0
    write_collider_time = 0
    first_spawn_id = 0
    
    for z, row in enumerate(cells):
        for x, cell in enumerate(row):
            write_mesh_time -= time.perf_counter()
            cell_data.append(generate_overworld_tile(
                cell,
                detail_cells[z][x],
                particle_cells[z][x],
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
                collider_cells[z][x][0].find_needed_edges()
                collider_cells[z][x][0].write_out(collider_data, force_subdivisions = mathutils.Vector((8, 1, 8)))
            else:
                tmp = mesh_collider.MeshCollider()
                tmp.write_out(collider_data, force_subdivisions=mathutils.Vector((8, 1, 8)))

            min_cell_min = mathutils.Vector((
                x * side_length + mesh_bb[0].x,
                mesh_bb[0].x,
                z * side_length + mesh_bb[0].z,
            ))
            
            entities.write_object_groups(
                min_cell_min, min_cell_min + mathutils.Vector((side_length, 0, side_length)),
                entity_cells[z][x],
                enums,
                variable_context,
                first_spawn_id,
                collider_data
            )

            first_spawn_id += len(entity_cells[z][x])

            collider_cell_data.append(collider_data.getvalue())
            write_collider_time += time.perf_counter()

    print(f"write_mesh_time = {write_mesh_time}")
    print(f"write_collider_time = {write_collider_time}")

    with open(overworld_filename, 'wb') as file:
        file.write('OVWD'.encode())

        file.write(struct.pack('>HH', subdivisions, subdivisions))
        file.write(struct.pack('>ff', mesh_bb[0].x, mesh_bb[0].z))
        file.write(struct.pack('>f', side_length))

        generate_lod0(lod_1_objects, subdivisions, settings, base_transform, file)

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