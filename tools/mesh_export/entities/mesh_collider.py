import bpy
import mathutils
import bmesh
import struct
import math

INDEX_BLOCK_SIZE_X = 8
INDEX_BLOCK_SIZE_Y = 16
INDEX_BLOCK_SIZE_Z = 8

ERROR_MARGIN = 0.001

class MeshColliderTriangle():
    def __init__(self, indices: list[float]):
        self.indices: list[float] = indices

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

def _determine_index_indices(points: list[mathutils.Vector]):
    min_point, max_point = _determine_index_bounds(points)
    stride_inv = mathutils.Vector((1 / INDEX_BLOCK_SIZE_X, 1 / INDEX_BLOCK_SIZE_Y, 1 / INDEX_BLOCK_SIZE_Z))
    size = (max_point - min_point) * stride_inv
    block_count = mathutils.Vector((math.ceil(size.x), math.ceil(size.y), math.ceil(size.z)))

    return min_point, stride_inv, block_count

class MeshIndexBlock():
    def __init__(self):
        self.indices: list[int] = []

class MeshIndex():
    def __init__(self, vertices: list[mathutils.Vector], triangles: list[MeshColliderTriangle]):
        self.vertices: list[mathutils.Vector] = vertices
        self.triangles: list[MeshColliderTriangle] = triangles

        self.blocks: list[MeshIndexBlock] = []

        min_point, stride_inv, block_count = _determine_index_indices(vertices)

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
        min_pos = self.vertices[triangle.indices[0]]
        max_pos = self.vertices[triangle.indices[0]]

        for idx in range(1, 3):
            min_pos = _vector_min(min_pos, self.vertices[triangle.indices[idx]])
            max_pos = _vector_max(max_pos, self.vertices[triangle.indices[idx]])

        min_block = _vector_floor(self._block_coordinates(min_pos) - mathutils.Vector((ERROR_MARGIN, ERROR_MARGIN, ERROR_MARGIN)))
        max_block = _vector_ceil(self._block_coordinates(max_pos) + mathutils.Vector((ERROR_MARGIN, ERROR_MARGIN, ERROR_MARGIN)))

        min_block = _vector_max(min_block, mathutils.Vector((0, 0, 0)))
        max_block = _vector_min(max_block, self.block_count)

        for z in range(int(min_block.z), int(max_block.z)):
            for y in range(int(min_block.y), int(max_block.y)):
                for x in range(int(min_block.x), int(max_block.x)):
                    self.check_triangle_to_box(x, y, z, triangle, triangle_index)


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

    def write_out(self, file):
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

        index = MeshIndex(self.vertices, self.triangles)
        index.serialize(file)