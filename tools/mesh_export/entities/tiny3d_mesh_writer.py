import struct

from . import mesh
from . import mesh_optimizer
from . import export_settings
from . import material_extract
from . import material_delta
from . import material
from . import serialize
from . import armature

MAX_BATCH_SIZE = 64

def triangle_vertices(data: mesh.mesh_data, triangle: int) -> list[int]:
    return [data.indices[index_index] for index_index in range(triangle, triangle + 3)]

def determine_space(current_indices: set[int], has_unused_vertex: set[int]) -> int:
    return MAX_BATCH_SIZE - len(current_indices) - len(has_unused_vertex)

class _mesh_pending_data():
    def __init__(self, data: mesh.mesh_data):
        self.data: mesh.mesh_data = data

        # maps vertex index to triangles that use the vertex
        self.vertex_triangle_usages: list[list[int]] = []

        for i in range(len(data.vertices)):
            self.vertex_triangle_usages.append([])

        self.triangle_count = len(data.indices) // 3

        for triangle_index in range(0, len(data.indices), 3):
            for offset in range(3):
                from_index = data.indices[triangle_index + offset]
                self.vertex_triangle_usages[from_index].append(triangle_index)
    
    def mark_triangle_used(self, triangle, current_indices: set[int], has_unused_vertex: set[int]):
        self.triangle_count -= 1
        for vertex_index in triangle_vertices(self.data, triangle):
            self.vertex_triangle_usages[vertex_index].remove(triangle)

            if not vertex_index in current_indices:
                bone_index = self.data.bone_indices[vertex_index]

                if bone_index in has_unused_vertex:
                    has_unused_vertex.remove(bone_index)
                else:
                    has_unused_vertex.add(bone_index)

    def determine_needed_vertex_count(self, triangle: int, current_indices: set[int], has_unused_vertex: set[int]) -> int:
        result: int = 0

        for vertex_index in triangle_vertices(self.data, triangle):
            if vertex_index in current_indices:
                continue

            bone_index = self.data.bone_indices[vertex_index]

            if bone_index in has_unused_vertex:
                # a space is already available 
                continue

            # 2 slots are needed since all vertices come in batches of 2
            # this is a constraint set by tiny3d
            result += 2

        return result

    def find_next_triangle(self, current_indices: set[int], has_unused_vertex: set[int]) -> int:
        triangle_result = -1
        needed_vertex_count = 0

        # search for adjacent vertices first
        for index in current_indices:
            for triangle in self.vertex_triangle_usages[index]:
                current_needed_vertex_count = self.determine_needed_vertex_count(triangle, current_indices, has_unused_vertex)

                if current_needed_vertex_count == 0:
                    self.mark_triangle_used(triangle, current_indices, has_unused_vertex)
                    return triangle
                
                if triangle_result == -1 or current_needed_vertex_count < needed_vertex_count:
                    needed_vertex_count = current_needed_vertex_count
                    triangle_result = triangle

        space = determine_space(current_indices, has_unused_vertex)

        # if no vertex was found adjacent to existing
        # vertices then search for any vertex
        if triangle_result == -1:
            for triangles in self.vertex_triangle_usages:
                for triangle in triangles:
                    current_needed_vertex_count = self.determine_needed_vertex_count(triangle, current_indices, has_unused_vertex)

                    if current_needed_vertex_count <= space:
                        self.mark_triangle_used(triangle, current_indices, has_unused_vertex)
                        return triangle

            return -1    
        
        if needed_vertex_count > space:
            return -1
        
        self.mark_triangle_used(triangle_result, current_indices, has_unused_vertex)

        return triangle_result

    
    def determine_usable_vertices(self, vertices: set[int]) -> list[int]:
        return set([x for x in vertices if len(self.vertex_triangle_usages[x]) > 0])
    
    def has_more(self):
        return self.triangle_count > 0
    
class _vertex_batch():
    def __init__(self, vertices: set[int], triangles: list[int]):
        self.vertices = vertices
        self.triangles = triangles

        self.vertex_lifetime: dict[int, int] = {}
        self.max_vertex_lifetime = 1

        for vertex in vertices:
            self.vertex_lifetime[vertex] = 1

    def check_vertex_life(self, next_vertices: set[int], offset: int) -> bool:
        for next_vertex in next_vertices:
            if next_vertex in self.vertex_lifetime and self.vertex_lifetime[next_vertex] == offset:
                self.vertex_lifetime[next_vertex] = offset + 1
                self.max_vertex_lifetime = offset + 1

        return self.max_vertex_lifetime > offset

class _batch_layout():
    def __init__(self, offset: int, new_indices: list[int], triangles: list[list[int]]):
        self.offset: int = offset
        self.new_indices: list[int] = new_indices
        self.triangles: list[list[int]] = triangles

def _group_vertices_by_bone(batch: _vertex_batch, mesh: mesh.mesh_data):
    by_bone: dict[int, list[int]] = {}

    for index in batch.vertices:
        bone_index = mesh.bone_indices[index]

        if bone_index in by_bone:
            by_bone[bone_index].append(index)
        else:
            by_bone[bone_index] = [index]

    for key in by_bone.keys():
        curr_list = by_bone[key]

        curr_list = sorted(curr_list, key = lambda index: -batch.vertex_lifetime[index])

        if len(curr_list) % 2 == 1:
            # batches much have an even number of vertices
            curr_list.append(curr_list[-1])

        by_bone[key] = curr_list

    groups = sorted(list(by_bone.values()), key = lambda index_list: -batch.vertex_lifetime[index_list[0]])

    return [x for group in groups for x in group]

class _vertex_layout():
    def __init__(self):
        self.current_indices: list[int] = [-1] * MAX_BATCH_SIZE
        self.current_index_life: list[int] = [1] * MAX_BATCH_SIZE

    def layout_batch(self, batch: _vertex_batch, mesh: mesh.mesh_data):
        ordered_vertices = _group_vertices_by_bone(batch, mesh)

        start_index = 0

        while start_index < MAX_BATCH_SIZE and self.current_indices[start_index] in batch.vertices \
            and self.current_index_life[start_index] >= batch.max_vertex_lifetime:
            start_index += 1

        # can only reuse in inrements of 2
        start_index = start_index & ~1

        end_index = MAX_BATCH_SIZE

        while end_index > 0 and self.current_indices[end_index - 1] in batch.vertices \
            and self.current_index_life[end_index - 1] >= batch.max_vertex_lifetime:
            end_index -= 1

        # can only reuse in increments of 2
        end_index = (end_index + 1) & ~1

        reused_indices = set(self.current_indices[0:start_index])

        for index in range(end_index, MAX_BATCH_SIZE):
            reused_indices.add(self.current_indices[index])

        new_indicies: list[int] = []

        for vertex_index in ordered_vertices:
            if vertex_index in reused_indices:
                continue

            new_indicies.append(vertex_index)

        if len(new_indicies) % 2 == 1:
            # batches much have an even number of vertices
            new_indicies.append(new_indicies[-1])

        offset = start_index

        if start_index < MAX_BATCH_SIZE - end_index:
            offset = end_index - len(new_indicies)
            new_indicies.reverse()

        for idx, index in enumerate(new_indicies):
            self.current_indices[idx + offset] = index
            self.current_index_life[idx + offset] = batch.vertex_lifetime[index] - 1

        triangles: list[list[int]] = []

        for triangle_index in batch.triangles:
            original_indices = map(lambda index: mesh.indices[index], range(triangle_index, triangle_index + 3))
            triangles.append(list(map(lambda index: self.current_indices.index(index), original_indices)))

        return _batch_layout(offset, new_indicies, triangles)
            
def _determine_triangle_order(data: mesh.mesh_data) -> list[_batch_layout]:
    pending_data = _mesh_pending_data(data)

    can_remove: set[int] = set()

    batches: list[_vertex_batch] = []

    while pending_data.has_more():
        current_batch: set[int] = set(can_remove)
        # determines which bones have an extra unused vertex
        has_unused_vertex: set[int] = set()
        triangles: list[int] = []

        while pending_data.has_more() and (determine_space(current_batch, has_unused_vertex) > 0 or len(can_remove) > 0):
            new_triangle = pending_data.find_next_triangle(current_batch, has_unused_vertex)

            if new_triangle == -1:
                if len(can_remove) == 0:
                    break
                
                current_batch.remove(can_remove.pop())
                continue
                
            for vertex in triangle_vertices(data, new_triangle):
                current_batch.add(vertex)
                if vertex in can_remove:
                    can_remove.remove(vertex)

            triangles.append(new_triangle)

        batches.append(_vertex_batch(current_batch, triangles))

        can_remove = pending_data.determine_usable_vertices(current_batch)

    for i in range(0, len(batches) - 1):
        next_index = i + 1

        batch = batches[i]

        while next_index < len(batches) and batch.check_vertex_life(batches[next_index].vertices, next_index - i):
            next_index += 1

    layout = _vertex_layout()

    result: list[_batch_layout] = []

    for batch in batches:
        result.append(layout.layout_batch(batch, data))

    return result

def _pack_pos(pos, settings: export_settings.ExportSettings):
    return struct.pack(
        '>hhh', 
        int(pos[0] * settings.fixed_point_scale),
        int(pos[1] * settings.fixed_point_scale),
        int(pos[2] * settings.fixed_point_scale)
    )

def _pack_rotation(rot):
    final_rot = rot if rot.w >= 0 else -rot
    return struct.pack(
        '>hhh', 
        round(32767 * final_rot.x),
        round(32767 * final_rot.y),
        round(32767 * final_rot.z)
    )

def _pack_normal(normal):
  x_int = int(round(normal[0] * 15.5))
  y_int = int(round(normal[1] * 15.5))
  z_int = int(round(normal[2] * 15.5))
  x_int = min(max(x_int, -16), 15)
  y_int = min(max(y_int, -16), 15)
  z_int = min(max(z_int, -16), 15)

  return ((x_int & 0b11111) << 10 | (y_int & 0b11111) <<  5 | (z_int & 0b11111) << 0).to_bytes(2, 'big')

def _pack_color(color):
    return struct.pack(
        '>BBBB', 
        int(pow(color[0], 1 / 2.2) * 255),
        int(pow(color[1], 1 / 2.2) * 255),
        int(pow(color[2], 1 / 2.2) * 255),
        int(pow(color[3], 1 / 2.2) * 255)
    )

def _pack_uv(uv, materail: material.Material):
    w, h = materail.get_image_size()

    return struct.pack(
        '>HH',
        round(uv[0] * w * 32) & 65535,
        round((1 - uv[1]) * h * 32) & 65535
    )

TMESH_COMMAND_VERTICES = 0
TMESH_COMMAND_TRIANGLES = 1
TMESH_COMMAND_MATERIAL = 2
TMESH_COMMAND_BONE = 3

def _build_vertices_command(source_index: int, offset: int, vertex_count: int):
    return struct.pack('>BBBH', TMESH_COMMAND_VERTICES, offset, vertex_count, source_index)

def _pack_vertices(chunk: mesh_optimizer.mesh_chunk, a: int, b: int, settings: export_settings.ExportSettings):
    mesh = chunk.data
    
    result = bytes()

    result += (_pack_pos(mesh.vertices[a], settings))
    result += (_pack_normal(mesh.normals[a]))
    
    has_second = b != -1

    if has_second:
        result += (_pack_pos(mesh.vertices[b], settings))
        result += (_pack_normal(mesh.normals[b]))
    else:
        result += (struct.pack('>hhhh', 0, 0, 0, 0))
    
    result += (_pack_color(mesh.color[a]))

    if has_second:
        result += (_pack_color(mesh.color[b]))
    else:
        result += (struct.pack('>BBBB', 0, 0, 0, 0))

    result += (_pack_uv(mesh.uv[a], chunk.material))
                
    if has_second:
        result += (_pack_uv(mesh.uv[b], chunk.material))
    else:
        result += (struct.pack('>hh', 0, 0))

    return result

def _build_triangles_command(indices: list[list[int]]):
    result = struct.pack('>BB', TMESH_COMMAND_TRIANGLES, len(indices))

    for triangle in indices:
        result += struct.pack('>BBB', triangle[0], triangle[1], triangle[2])

    return result

def _build_material_command(material_index: int):
    return struct.pack('>BH', TMESH_COMMAND_MATERIAL, material_index)

def _build_bone_command(bone_index: int):
    return struct.pack('>Bh', TMESH_COMMAND_BONE, bone_index)

def _write_mesh_chunk(chunk: mesh_optimizer.mesh_chunk, settings: export_settings.ExportSettings, command_list: list[bytes], vertices: list[bytes], current_bone: int) -> int:
    batch_layouts = _determine_triangle_order(chunk.data)

    for batch in batch_layouts:
        source_index = len(vertices)

        vertex_count = (len(batch.new_indices) + 1) & ~1

        last_start = 0

        for index in range(vertex_count):
            bone_index = chunk.data.bone_indices[batch.new_indices[index]] 

            if bone_index != current_bone:
                if last_start != index:
                    command_list.append(_build_vertices_command(source_index + last_start // 2, batch.offset + last_start, index - last_start))

                command_list.append(_build_bone_command(bone_index))
                current_bone = bone_index
                last_start = index
            
        command_list.append(_build_vertices_command(source_index + last_start // 2, batch.offset + last_start, vertex_count - last_start))

        for i in range(0, vertex_count, 2):
            vertices.append(_pack_vertices(
                chunk, 
                batch.new_indices[i], batch.new_indices[i + 1] if i + 1 < len(batch.new_indices) else -1,
                settings
            ))

        command_list.append(_build_triangles_command(batch.triangles))

    return current_bone
        

def write_mesh(mesh_list: list[tuple[str, mesh.mesh_data]], arm: armature.ArmatureData | None, attatchments: list[armature.BoneLinkage], settings: export_settings.ExportSettings, file):
    file.write('T3MS'.encode())

    chunks = []
    
    for mesh in mesh_list:
        mat = material_extract.load_material_with_name(mesh[0], mesh[1].mat)
        chunks += mesh_optimizer.chunkify_mesh(mesh[1], mat, settings.default_material)

    chunks = mesh_optimizer.determine_chunk_order(chunks, settings.default_material)

    commands = []
    vertices = []

    current_material = material.Material()

    if settings.default_material_name != None and settings.default_material_name.startswith('materials/'):
        current_material = settings.default_material
        material_romname = f"rom:/{settings.default_material_name}.mat".encode()
        file.write(len(material_romname).to_bytes(1, 'big'))
        file.write(material_romname)
    else:
        # no material specified
        print("invalid material name settings.default_material_name ", settings.default_material_name)
        file.write((0).to_bytes(1, 'big'))

    material_delta.apply_material_delta(settings.default_material, current_material)

    material_transitions: list[tuple[material.Material, material.Material]] = []

    current_bone = -1

    for chunk in chunks:
        # if chunk.material.tex0 and chunk.material.tex0.filename and chunk.material.tex0.filename.endswith('cloth.png'):
        #     print(chunk.used_bones)
        #     bones = arm.get_filtered_bones()
        #     print([bones[index].name for index in list(chunk.used_bones)])

        delta = material_delta.determine_material_delta(current_material, chunk.material)

        if not delta.is_empty():
            commands.append(_build_material_command(len(material_transitions)))
            material_delta.apply_material_delta(delta, current_material)

            curr_copy = material.Material()
            material_delta.apply_material_delta(current_material, curr_copy)
            material_transitions.append((delta, curr_copy))

        current_bone = _write_mesh_chunk(chunk, settings, commands, vertices, current_bone)

    file.write(len(vertices).to_bytes(2, 'big'))

    for vertex in vertices:
        file.write(vertex)

    file.write(len(material_transitions).to_bytes(2, 'big'))

    for transition in material_transitions:
        serialize.serialize_material_file(file, transition[0], transition[1])

    armature.write_armature(file, arm, settings)

    file.write(len(attatchments).to_bytes(2, 'big'))
    for linkage in attatchments:
        name = linkage.name.encode()
        file.write(len(name).to_bytes(1, 'big'))
        file.write(name)

        file.write(linkage.bone_index.to_bytes(2, 'big'))
        loc, rot, scale = linkage.transform.decompose()
        file.write(_pack_pos(loc, settings))
        file.write(_pack_rotation(rot))
        file.write(struct.pack(">h", int(scale.x * 256)))
        

    file.write(len(commands).to_bytes(2, 'big'))

    for command in commands:
        file.write(command)