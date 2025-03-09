import bpy
import mathutils
import bmesh
import struct
import math
import time

INDEX_BLOCK_SIZE_X = 8
INDEX_BLOCK_SIZE_Y = 16
INDEX_BLOCK_SIZE_Z = 8

ERROR_MARGIN = 0.001

class MeshColliderTriangle():
    def __init__(self, indices: list[int]):
        self.indices: list[int] = indices

def _triangle_support_function(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle, direction: mathutils.Vector):
    result_index = max(triangle.indices, key = lambda index: direction.dot(vertices[index]))
    return vertices[result_index]

def _aabb_support_function(min: mathutils.Vector, max: mathutils.Vector, direction: mathutils.Vector):
    return mathutils.Vector((
        min.x if direction.x < 0 else max.x,
        min.y if direction.y < 0 else max.y,
        min.z if direction.z < 0 else max.z
    ))

_basis_vectors = [
    mathutils.Vector((1, 0, 0)),
    mathutils.Vector((0, 1, 0)),
    mathutils.Vector((0, 0, 1))
]

def _generate_overlap_directions(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle) -> list[mathutils.Vector]:
    triangle_positions: list[mathutils.Vector] = [vertices[index] for index in triangle.indices]

    result: list[mathutils.Vector] = []

    result.append((triangle_positions[1] - triangle_positions[0]).cross(triangle_positions[2] - triangle_positions[0]))

    for basis_vector in _basis_vectors:
        result.append(basis_vector)

        for idx in range(len(triangle_positions)):
            next_idx = (idx + 1) % 3
            result.append(basis_vector.cross(triangle_positions[idx] - triangle_positions[next_idx]))

    return result

def _does_overlap(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle, min: mathutils.Vector, max: mathutils.Vector):
    vector_directions = _generate_overlap_directions(vertices, triangle)

    for vector_direction in vector_directions:
        if vector_direction.magnitude < 0.0000001:
            continue

        dir = vector_direction.normalized()

        max_tri = dir.dot(_triangle_support_function(vertices, triangle, dir))
        min_tri = dir.dot(_triangle_support_function(vertices, triangle, -dir))

        max_box = dir.dot(_aabb_support_function(min, max, dir))
        min_box = dir.dot(_aabb_support_function(min, max, -dir))

        if min_tri > max_box + ERROR_MARGIN or min_box > max_tri + ERROR_MARGIN:
            return False

    return True

def _vector_min(a: mathutils.Vector, b: mathutils.Vector):
    return mathutils.Vector((
            min(a.x, b.x), 
            min(a.y, b.y), 
            min(a.z, b.z),
        ))

def _vector_max(a: mathutils.Vector, b: mathutils.Vector):
    return mathutils.Vector((
            max(a.x, b.x), 
            max(a.y, b.y), 
            max(a.z, b.z),
        ))

def _vector_floor(point: mathutils.Vector):
    return mathutils.Vector((
            math.floor(point.x),
            math.floor(point.y),
            math.floor(point.z),
        ))

def _vector_ceil(point: mathutils.Vector):
    return mathutils.Vector((
            math.ceil(point.x),
            math.ceil(point.y),
            math.ceil(point.z),
        ))

def _determine_index_bounds(points: list[mathutils.Vector]):
    if len(points) == 0:
        return mathutils.Vector(), mathutils.Vector()

    min_point = points[0]
    max_point = points[0]

    for point in points:
        min_point = _vector_min(min_point, _vector_floor(point))
        max_point = _vector_max(max_point, _vector_ceil(point))

    return min_point, max_point

def _determine_index_indices(points: list[mathutils.Vector], force_subdivisions: mathutils.Vector | None = None):
    min_point, max_point = _determine_index_bounds(points)

    if force_subdivisions:
        size = max_point - min_point
        stride_inv = mathutils.Vector((force_subdivisions.x / size.z, force_subdivisions.y / size.y, force_subdivisions.z / size.z))
        return min_point, stride_inv, force_subdivisions

    stride_inv = mathutils.Vector((1 / INDEX_BLOCK_SIZE_X, 1 / INDEX_BLOCK_SIZE_Y, 1 / INDEX_BLOCK_SIZE_Z))
    size = (max_point - min_point) * stride_inv
    block_count = mathutils.Vector((math.ceil(size.x), math.ceil(size.y), math.ceil(size.z)))

    return min_point, stride_inv, block_count

CENTER_THREHOLD = 0.001

def _split_loop(loop: list[mathutils.Vector], axis: int, distance: float) -> tuple[list[mathutils.Vector], list[mathutils.Vector]]:
    offsets = list(map(lambda x: x[axis] - distance, loop))

    behind = []
    front = []

    for i in range(len(loop)):
        current_offset = offsets[i]

        if current_offset < CENTER_THREHOLD:
            behind.append(loop[i])

        if current_offset > -CENTER_THREHOLD:
            front.append(loop[i])


        next_index = (i + 1) % len(loop)
        next_offset = offsets[next_index]

        if (current_offset >= CENTER_THREHOLD and next_offset <= -CENTER_THREHOLD) or (current_offset <= -CENTER_THREHOLD and next_offset >= CENTER_THREHOLD):
            lerp_value = -current_offset / (next_offset - current_offset)
            center_point = loop[i] * (1 - lerp_value) + loop[next_index] * lerp_value
            behind.append(center_point)
            front.append(center_point)

    return behind, front

def _rasterize_loop_detrmine_range(points: list[mathutils.Vector], axis: int) -> tuple[int, int]:
    return min(map(lambda point: math.floor(point[axis]), points)), max(map(lambda point: math.ceil(point[axis]), points))

def _rasterize_loop_axis_3d(points: list[mathutils.Vector], axis: int, result: list[tuple[int, int, int]], prev_axis: int | None):
    curr_value = min(map(lambda point: math.floor(point[axis]), points))

    curr_loop = points

    while len(curr_loop) > 0:
        behind, front = _split_loop(curr_loop, axis, curr_value + 1)

        if len(behind) == 0:
            raise Exception('the behind loop should never be empty')

        if axis == 1:
            min_z, max_z = _rasterize_loop_detrmine_range(behind, 2)
            for z in range(min_z, max_z):
                result.append((prev_axis, curr_value, z))
        else:
            _rasterize_loop_axis_3d(behind, axis + 1, result, curr_value)

        curr_value += 1
        curr_loop = front

        if curr_value > 100:
            print(curr_loop)
            raise Exception('infinite loop here')

    
def _rasterize_triangle_3d(points: list[mathutils.Vector]) -> list[tuple[int, int, int]]:
    result = []
    _rasterize_loop_axis_3d(points, 0, result, None)
    return result


class MeshIndexBlock():
    def __init__(self):
        self.indices: list[int] = []

class MeshIndex():
    def __init__(self, vertices: list[mathutils.Vector], triangles: list[MeshColliderTriangle], force_subdivisions: mathutils.Vector | None = None):
        self.vertices: list[mathutils.Vector] = vertices
        self.triangles: list[MeshColliderTriangle] = triangles

        self.blocks: list[MeshIndexBlock] = []

        min_point, stride_inv, block_count = _determine_index_indices(vertices, force_subdivisions = force_subdivisions)

        self.min_point: mathutils.Vector = min_point
        self.stride_inv: mathutils.Vector = stride_inv
        self.block_count: mathutils.Vector = block_count

        count = int(self.block_count.x * self.block_count.y * self.block_count.z)

        for idx in range(count):
            self.blocks.append(MeshIndexBlock())

        for idx in range(len(self.triangles)):
            self.check_triangle(idx)

    def __str__(self):
        block_indices = []

        for block in self.blocks:
            block_indices.append(', '.join([str(idx) for idx in block.indices]))

        indices = '\n'.join(block_indices)

        return f"min = {self.min_point}\nstride_inv = {self.stride_inv}\nblock_count = {self.block_count}\nblocks = {len(self.blocks)}\n{indices}"
    
    def serialize(self, file):
        file.write(struct.pack(">fff", self.min_point.x, self.min_point.y, self.min_point.z))
        file.write(struct.pack(">fff", self.stride_inv.x, self.stride_inv.y, self.stride_inv.z))
        file.write(struct.pack(">BBB", int(self.block_count.x), int(self.block_count.y), int(self.block_count.z)))

        index_count = 0

        for block in self.blocks:
            index_count += len(block.indices)

        file.write(struct.pack(">H", index_count))

        for block in self.blocks:
            for index in block.indices:
                file.write(struct.pack(">H", index))

        current_index = 0

        for block in self.blocks:
            file.write(struct.pack(">HH", current_index, current_index + len(block.indices)))
            current_index += len(block.indices)



    def _block_coordinates(self, input: mathutils.Vector) -> mathutils.Vector:
        return (input - self.min_point) * self.stride_inv
    
    def check_triangle_to_box(self, x: int, y: int, z: int, triangle: MeshColliderTriangle, triangle_index: int):
        min_box = mathutils.Vector((
            x * INDEX_BLOCK_SIZE_X + self.min_point.x,
            y * INDEX_BLOCK_SIZE_Y + self.min_point.y,
            z * INDEX_BLOCK_SIZE_Z + self.min_point.z,
        ))

        max_box = mathutils.Vector((
            min_box.x + INDEX_BLOCK_SIZE_X,
            min_box.y + INDEX_BLOCK_SIZE_Y,
            min_box.z + INDEX_BLOCK_SIZE_Z,
        ))

        if not _does_overlap(self.vertices, triangle, min_box, max_box):
            return
        
        index = int(x + ((y + z * self.block_count.y) * self.block_count.x))

        self.blocks[index].indices.append(triangle_index)

    def check_triangle(self, triangle_index: int):
        triangle: MeshColliderTriangle = self.triangles[triangle_index]

        block_vertices = list(map(lambda index: self._block_coordinates(self.vertices[index]), triangle.indices))
        block_indices = _rasterize_triangle_3d(block_vertices)
        
        for cooridnate in block_indices:
            if cooridnate[0] < 0 or cooridnate[0] >= self.block_count[0] or \
                cooridnate[1] < 0 or cooridnate[1] >= self.block_count[1] or \
                cooridnate[2] < 0 or cooridnate[2] >= self.block_count[2]:
                continue

            index = int(cooridnate[0] + ((cooridnate[1] + cooridnate[2] * self.block_count.y) * self.block_count.x))
            self.blocks[index].indices.append(triangle_index)

class MeshCollider():
    def __init__(self):
        self.vertices: list[mathutils.Vector] = []
        self.triangles: list[MeshColliderTriangle] = []

    def append(self, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)

        for face in bm.faces:
            face.verts
            face.loops

        start_index = len(self.vertices)

        for vert in bm.verts:
            self.vertices.append(transform @ vert.co)

        for face in bm.faces:
            self.triangles.append(MeshColliderTriangle([
                face.verts[0].index + start_index,
                face.verts[1].index + start_index,
                face.verts[2].index + start_index,
            ]))

        bm.free()

    def write_out(self, file, force_subdivisions: mathutils.Vector | None = None):
        file.write('CMSH'.encode())
        file.write(len(self.vertices).to_bytes(2, 'big'))

        for vert in self.vertices:
            file.write(struct.pack(
                ">fff",
                vert.x,
                vert.y,
                vert.z
            ))

        file.write(len(self.triangles).to_bytes(2, 'big'))

        for triangle in self.triangles:
            file.write(struct.pack(
                ">HHH",
                triangle.indices[0],
                triangle.indices[1],
                triangle.indices[2],
            ))

        index = MeshIndex(self.vertices, self.triangles, force_subdivisions = force_subdivisions)
        index.serialize(file)

    def is_empty(self):
        return len(self.triangles) == 0
    
    def copy_blank(self):
        return MeshCollider()
    
    def append_vertex(self, vertex_data):
        self.vertices.append(vertex_data)

    def append_triangle(self, a, b, c):
        self.triangles.append(MeshColliderTriangle([a, b, c]))

    def get_triangles(self):
        result: list[list[int]] = []

        for triangle in self.triangles:
            result.append(triangle.indices)

        return result
    
    def get_vertex(self, index):
        return self.vertices[index]
    
    def get_vertex_interpolated(self, a, b, lerp):
        lerp_inv = 1 - lerp
        return self.vertices[a] * lerp_inv + self.vertices[b] * lerp