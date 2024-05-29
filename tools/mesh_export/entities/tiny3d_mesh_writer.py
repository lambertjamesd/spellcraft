import struct

from . import mesh
from . import mesh_optimizer
from . import export_settings
from . import material

def add_to_adjacency(adjacency: dict[int, set[int]], from_index: int, to_index: int):
    if from_index in adjacency:
        adjacency[from_index].add(to_index)
    else:
        adjacency[from_index] = {to_index}

class _mesh_pending_data():
    def __init__(self, data: mesh.mesh_data):
        self.data: mesh.mesh_data = data

        # maps vertex index to triangles that use the vertex
        self.vertex_triangle_usages: list[list[int]] = []

        for i in range(len(data.vertices)):
            self.vertex_triangle_usages.append([])

        self.adjacency: dict[int, set[int]] = {}

        self.triangle_count = len(data.indices) // 3

        for triangle_index in range(0, len(data.indices), 3):
            for offset in range(3):
                from_index = data.indices[triangle_index + offset]
                to_index = data.indices[(triangle_index + offset + 1) % 3]

                self.vertex_triangle_usages[from_index].append(triangle_index)

                add_to_adjacency(self.adjacency, from_index, to_index)
                add_to_adjacency(self.adjacency, to_index, from_index)

                # add to adjacency

    def is_triangle_complete(self, triangle: int, current_indices: set[int], next: int):
        triangle_indices = map(lambda index: self.data.indices[index], range(triangle, triangle + 3))
        return all(map(lambda index: index in current_indices or index == next, triangle_indices))
    
    def is_triangle_nearly_complete(self, triangle: int, current_indices: set[int]):
        triangle_indices = map(lambda index: self.data.indices[index], range(triangle, triangle + 3))
        return any(map(lambda index: index in current_indices, triangle_indices))

    def find_candidate(self, current_indices: set[int]) -> tuple[int, list[int]]:
        if len(current_indices) == 0:
            idx = -1
            max_triangles = 0

            for i, usage in enumerate(self.vertex_triangle_usages):
                if idx == -1 or len(usage) >  max_triangles:
                    max_triangles = len(usage)
                    idx = i

            return idx, []

        adjacent_set = set()
        for index in current_indices:
            adjacent_set = adjacent_set.union(self.adjacency[index])
        
        adjacent_set = adjacent_set.difference(current_indices)

        result = -1
        completed_triangles = []
        incomplete_triangle_count = 0

        for adjacent in adjacent_set:
            current_completed_triangles = []
            current_incomplete_triangle_count = 0

            for triangle in self.vertex_triangle_usages[adjacent]:
                if self.is_triangle_complete(triangle, current_indices, adjacent):
                    current_completed_triangles.append(triangle)
                elif self.is_triangle_nearly_complete(triangle, current_indices):
                    current_incomplete_triangle_count += 1

            if result == -1 or len(current_completed_triangles) > len(completed_triangles) or \
                (len(current_completed_triangles) == len(completed_triangles) and current_incomplete_triangle_count > incomplete_triangle_count):
                completed_triangles = current_completed_triangles
                result = adjacent
                incomplete_triangle_count = current_incomplete_triangle_count

        if result == -1:
            return -1, []

        self.vertex_triangle_usages[result] = [triangle for triangle in self.vertex_triangle_usages[result] if not triangle in completed_triangles]

        self.triangle_count -= len(completed_triangles)

        return result, completed_triangles
    
    def determine_best_removal(self, current_indices: set[int], can_remove: set[int], triangles: list[int]):
        for vertex_index in can_remove:
            if not vertex_index in current_indices:
                continue

            if any(map(lambda triangle: any(map(lambda index: vertex_index == self.data.indices[index], range(triangle, triangle + 3))), triangles)):
                continue

            return vertex_index

        return -1
    
    def determine_usable_vertices(self, vertices: set[int]) -> set[int]:
        return set([x for x in vertices if len(self.vertex_triangle_usages[x] > 0)])
    
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

    def check_vertex_life(self, vertices: set[int], offset: int) -> bool:
        for vertex in vertices:
            if vertex in self.vertex_lifetime and self.vertex_lifetime[vertex] == offset:
                self.vertex_lifetime[vertex] = offset + 1
                self.max_vertex_lifetime = offset + 1

        return self.max_vertex_lifetime > offset
    
MAX_BATCH_SIZE = 64

class _batch_layout():
    def __init__(self, offset: int, new_indices: list[int], triangles: list[list[int]]):
        self.offset: int = offset
        self.new_indices: list[int] = new_indices
        self.traingles: list[list[int]] = triangles

class _vertex_layout():
    def __init__(self):
        self.current_indices: list[int] = [-1] * MAX_BATCH_SIZE
        self.current_index_life: list[int] = [1] * MAX_BATCH_SIZE

    def layout_batch(self, batch: _vertex_batch, mesh: mesh.mesh_data):
        ordered_vertices = sorted(batch.vertices, key = lambda index: -batch.vertex_lifetime[index])

        start_index = 0

        while start_index < MAX_BATCH_SIZE and self.current_indices[start_index] in batch.vertices \
            and self.current_index_life[start_index] >= batch.max_vertex_lifetime:
            start_index += 1

        end_index = MAX_BATCH_SIZE

        while end_index > 0 and self.current_indices[end_index - 1] in batch.vertices \
            and self.current_index_life[end_index - 1] >= batch.max_vertex_lifetime:
            end_index -= 1

        reused_indices = set(self.current_indices[0:start_index])

        for index in range(end_index, MAX_BATCH_SIZE):
            reused_indices.add(self.current_indices[index])

        new_indicies = [index for index in ordered_vertices if not index in reused_indices]
        offset = start_index

        if start_index < MAX_BATCH_SIZE - end_index:
            offset = end_index - len(new_indicies)

        for idx, index in enumerate(new_indicies):
            self.current_indices[idx + offset] = index
            self.current_index_life[idx + offset] = batch.vertex_lifetime[index] - 1

        triangles: list[list[int]] = []

        for triangle_index in batch.triangles:
            original_indices = map(lambda index: mesh.indices[index], range(triangle_index, triangle_index + 3))
            triangles.append(map(lambda index: self.current_indices.index(index), original_indices))

        return _batch_layout(offset, new_indicies, triangles)
            
def _determine_triangle_order(data: mesh.mesh_data) -> list[_batch_layout]:
    pending_data = _mesh_pending_data(data)

    prev_batch: set[int] | None = None

    batches: list[_vertex_batch] = []

    while pending_data.has_more():
        current_batch: set[int] = set()
        triangles: list[int] = []

        if prev_batch:
            current_batch = pending_data.determine_usable_vertices(prev_batch)
        
        while len(current_batch) < MAX_BATCH_SIZE:
            vertex, new_triangles = pending_data.find_candidate(current_batch)

            current_batch.add(vertex)
            triangles += new_triangles

            if len(current_batch) == MAX_BATCH_SIZE:
                best_removal = pending_data.determine_best_removal(current_batch, prev_batch, triangles)

                if best_removal != -1:
                    current_batch.remove(best_removal)

        batches.append(_vertex_batch(current_batch, triangles))

        prev_batch = set(current_batch)

    for i in range(0, len(batches) - 1):
        next_index = i + 1

        batch = batches[i]

        while batch.check_vertex_life(batches[next_index].vertices, next_index - i):
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

def _pack_normal(normal):
  x_int = int(round(normal[0] * 15.5))
  y_int = int(round(normal[1] * 15.5))
  z_int = int(round(normal[2] * 15.5))
  x_int = min(max(x_int, -16), 15)
  y_int = min(max(y_int, -16, 15))
  z_int = min(max(z_int, -16, 15))

  return ((x_int & 0b11111) << 10 | (y_int & 0b11111) <<  5 | (z_int & 0b11111) << 0).to_bytes(2, 'big')

def _pack_color(color, settings: export_settings.ExportSettings):
    return struct.pack(
        '>BBBB', 
        int(color[0] * 255),
        int(color[1] * 255),
        int(color[2] * 255),
        int(color[3] * 255)
    )

def _pack_uv(uv):
    return struct.pack(
        '>hh',
        # TODO multiply by the texture size
        round(uv[0] * 32 * 32),
        round(uv[0] * 32 * 32)
    )

def _build_vertices_command(source_index: int, offset: int, vertex_count: int):
    return struct.pack('>BBBH', 0, offset, vertex_count, source_index)

def _pack_vertices(mesh: mesh.mesh_data, a: int, b: int, settings: export_settings.ExportSettings):
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

    result += (_pack_uv(mesh.uv[a]))
                
    if has_second:
        result += (_pack_uv(mesh.uv[b]))
    else:
        result += (struct.pack('>hh', 0, 0))

def _build_triangles_command(indices: list[list[int]]):
    result = struct.pack('>BB', 0, len(indices))

    for triangle in indices:
        result += struct.pack('>BBB', triangle[0], triangle[1], triangle[2])

    return result

def _write_mesh_chunk(chunk: mesh_optimizer.mesh_chunk, settings: export_settings.ExportSettings, command_list: list[bytes], vertices: list[bytes]):
    mesh = chunk.data

    batch_layouts = _determine_triangle_order(mesh)

    for batch in batch_layouts:
        source_index = len(vertices)

        vertex_count = (len(batch.new_indices) + 1) & ~1

        command_list.append(_build_vertices_command(source_index, batch.offset, vertex_count))

        for i in range(0, vertex_count, 2):
            vertices.append(_pack_vertices(
                mesh, 
                batch.new_indices[i], batch.new_indices[i + 1] if i + 1 < len(batch.new_indices) else -1,
                settings
            ))

        command_list.append(_build_triangles_command(batch.traingles))
        

def write_mesh(mesh_list: list[tuple[str, mesh.mesh_data]], settings: export_settings.ExportSettings, file):
    file.write('T3MS'.encode())

    chunks = []
    
    for mesh in mesh_list:
        mat = material.load_material_with_name(mesh[0], mesh[1].mat)
        chunks += mesh_optimizer.chunkify_mesh(mesh[1], mat, settings.default_material)

    chunks = mesh_optimizer.determine_chunk_order(chunks, settings.default_material)

    commands = []
    vertices = []

    for chunk in chunks:
        _write_mesh_chunk(chunk, settings, commands, vertices)

    file.write(len(vertices).to_bytes(2, 'big'))

    for vertex in vertices:
        file.write(vertex)

    file.write(len(commands).to_bytes(2, 'big'))

    for command in commands:
        file.write(command)