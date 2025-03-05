import mathutils
import io
import struct
from . import mesh
from . import bounding_box
from . import mesh_split
from . import tiny3d_mesh_writer
from . import export_settings

def subdivide_mesh_list(meshes: list[tuple[str,mesh.mesh_data]], normal: mathutils.Vector, start_pos: float, distance_step: float, subdivisions: int) -> list[list[tuple[str,mesh.mesh_data]]]:
    result = []
    distance = -(distance_step + start_pos)

    for i in range(subdivisions - 1):
        chunk = []
        next = []

        for mesh in meshes:
            behind, front = mesh_split.split(mesh[1], normal, distance)

            if behind:
                chunk.append((mesh[0], behind))

            if front:
                next.append((mesh[0], front))
        
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

def generate_overworld(overworld_filename: str, mesh_list: mesh.mesh_list, lod_0_mesh: mesh.mesh_list, subdivisions: int, settings: export_settings.ExportSettings):
    mesh_entries = mesh_list.determine_mesh_data()

    mesh_bb = None

    for entry in mesh_entries:
        entry_bb = entry[1].bounding_box()

        if mesh_bb:
            mesh_bb = bounding_box.union(mesh_bb, entry_bb)
        else:
            mesh_bb = entry_bb

    width = mesh_bb[1].x - mesh_bb[0].x
    height = mesh_bb[1].z - mesh_bb[0].z

    side_length = max(width, height) / subdivisions

    columns = []

    columns = subdivide_mesh_list(mesh_entries, mathutils.Vector((0, 0, 1)), mesh_bb[0].x, side_length, subdivisions)
    cells = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].z, side_length, subdivisions), columns))

    cell_data: list[OverworldCell] = []
    
    for y, row in enumerate(cells):
        for x, cell in enumerate(row):
            cell_bb = cell[0][1].bounding_box()

            for i in range(1, len(cell)):
                cell_bb = bounding_box.union(cell_bb, cell[i][1].bounding_box())

            height = cell_bb[1].y - cell_bb[0].y
            y_scale = max_block_height / height if height > max_block_height else 1

            for mesh_data in cell:
                mesh_data[1].translate(mathutils.Vector((
                    -(x * side_length + mesh_bb[0].x), 
                    -cell_bb[0].y, 
                    -(y * side_length + mesh_bb[0].z)
                )))
                mesh_data[1].scale(mathutils.Vector((
                    1, 
                    y_scale, 
                    1
                )))

            data = io.BytesIO()
            tiny3d_mesh_writer.write_mesh(cell, None, [], settings, data)
            data.write(struct.pack('>ff', cell_bb[0].y, y_scale))
            cell_data.append(OverworldCell(data.getvalue(), cell_bb[0].y, y_scale))

    lod_0_mesh_bytes = io.BytesIO()
    lod_0_settings = settings.copy()
    lod_0_settings.sort_direction = mathutils.Vector((1, 0, 0))
    lod_0_mesh_data = lod_0_mesh.determine_mesh_data(None)
    for entry in lod_0_mesh_data:
        entry[1].scale(LOD_0_SCALE)
    tiny3d_mesh_writer.write_mesh(lod_0_mesh_data, None, [], lod_0_settings, lod_0_mesh_bytes)

    with open(overworld_filename, 'wb') as file:
        file.write('OVWD'.encode())

        file.write(struct.pack('>HH', subdivisions, subdivisions))
        file.write(struct.pack('>ff', mesh_bb[0].x, mesh_bb[0].z))
        file.write(struct.pack('>f', side_length))

        for i in range(4):
            file.write(lod_0_mesh_bytes.getvalue())

        current_location = file.tell() + 4 * subdivisions * subdivisions

        for cell in cell_data:
            file.write(struct.pack('>I', current_location))
            current_location += len(cell.mesh_data)

        for cell in cell_data:
            file.write(cell.mesh_data)