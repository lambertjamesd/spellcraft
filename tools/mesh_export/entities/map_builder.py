import bpy
import mathutils
import collections
import math
import struct

from . import bounding_box
from . import mesh
from . import tiny3d_mesh_writer
from . import export_settings

TARGET_MAP_SIZE = 200
FIXED_POINT_SCALE = 4
MAX_ZOOM = 4

TOTAL_SCALE = TARGET_MAP_SIZE * FIXED_POINT_SCALE * MAX_ZOOM

LINE_WIDTH = 2

class MapEntry():
    def __init__(self, obj: bpy.types.Object, room_index: int):
        self.obj: bpy.types.Object = obj
        self.room_index: int = room_index

    def bounding_box(self) -> tuple[mathutils.Vector, mathutils.Vector]:
        return bounding_box.from_obj(self.obj)

def rotate_complex(a: mathutils.Vector, rotation: mathutils.Vector) -> mathutils.Vector:
    return mathutils.Vector((a.x * rotation.x - a.y * rotation.y, a.x * rotation.y + a.y * rotation.x, a.z))

def calculate_joint(a: mathutils.Vector, b: mathutils.Vector, c: mathutils.Vector, scale: float) -> list[mathutils.Vector]:
    relative_to = b - a
    relative_to.z = 0
    relative_to.normalize()

    relative_to_inv = mathutils.Vector((relative_to.x, -relative_to.y, relative_to.z))

    dir = (c - b).normalized()

    dir = rotate_complex(dir, relative_to_inv)

    offset = 1 - dir.x

    local_position: list[mathutils.Vector] = []

    if abs(dir.y) < 0.001:
        local_position.append(mathutils.Vector((0, -1, 0)))
        if dir.x < 0:
            local_position.append(mathutils.Vector((0, 1, 0)))
    else:
        time = offset / -dir.y
        x = dir.x * time + dir.y
        local_position.append(mathutils.Vector((x, -1, 0)))

    result: list[mathutils.Vector] = []

    for entry in local_position:
        result.append(rotate_complex(entry, relative_to) * scale + b)

    return result

def other_vertex(edge: bpy.types.MeshEdge, index: int) -> int:
    return edge.vertices[1] if edge.vertices[0] == index else edge.vertices[0]

class Pen():
    def __init__(self, vertices: list[mathutils.Vector]):
        self.vertex_index: int = -1
        self.first_vertex: int = -1
        self.vertices: list[mathutils.Vector] = vertices

    def move_to(self, next_index: int):
        self.vertex_index = next_index

        if self.first_vertex == -1:
            self.first_vertex = next_index

    def location(self) -> mathutils.Vector:
        return self.vertices[self.vertex_index] if self.vertex_index != -1 else mathutils.Vector()

def create_vertex(pos: mathutils.Vector, alpha, x_uv):
    return [
        pos,
        mathutils.Vector(),
        [1, 1, 1, alpha],
        [x_uv, 0],
        0,
    ]

def write_outline(into: mesh.mesh_data, obj: bpy.types.Object, global_transform: mathutils.Matrix):
    mesh_transform = global_transform @ obj.matrix_world

    if not isinstance(obj.data, bpy.types.Mesh):
        raise Exception('write_outline expects an mesh object')
    
    mesh = obj.data

    edge_use_count: dict[str, int] = {}

    for poly in mesh.polygons:
        for edge_key in poly.edge_keys:
            if edge_key in edge_use_count:
                edge_use_count[edge_key] = edge_use_count[edge_key] + 1
            else:
                edge_use_count[edge_key] = 1
    
    remaining_edges: list[bpy.types.MeshEdge] = []
    for edge in mesh.edges:
        if edge.key in edge_use_count:
            adj_edges = edge_use_count[edge.key]
        else:
            adj_edges = 0

        if adj_edges > 1:
            continue

        remaining_edges.append(edge)

    vertex_to_edge = collections.defaultdict(list)

    for edge in remaining_edges:
        v1, v2 = edge.vertices
        vertex_to_edge[v1].append(edge)
        vertex_to_edge[v2].append(edge)

    vertices = [mesh_transform @ vtx.co for vtx in mesh.vertices]
    joints: dict[int, list[list[mathutils.Vector]]] = {}

    for vertex_index, edges in vertex_to_edge.items():
        pivot = vertices[vertex_index]

        def other_point(edge: bpy.types.MeshEdge) -> mathutils.Vector:
            return vertices[other_vertex(edge, vertex_index)]

        def edge_angle(edge: bpy.types.MeshEdge) -> float:
            dir = other_point(edge) - pivot
            return math.atan2(dir.y, dir.x)

        edges.sort(key=edge_angle)

        joints_for_edges: list[list[mathutils.Vector]] = []

        for edge_index, edge in enumerate(edges):
            next_edge = edges[(edge_index + 1) % len(edges)]
            joints_for_edges.append(calculate_joint(other_point(edge), pivot, other_point(next_edge), LINE_WIDTH * FIXED_POINT_SCALE * MAX_ZOOM))

        joints[vertex_index] = joints_for_edges

    consumed_edges = set()

    pen = Pen(vertices)

    def move_pen(edge: bpy.types.MeshEdge, next_index: int):
        pen.move_to(next_index)
        remaining_edges.remove(edge)
        consumed_edges.add(edge.key)

    def find_next_edge() -> bpy.types.MeshEdge:
        if pen.vertex_index != -1:
            edges = vertex_to_edge[pen.vertex_index]

            for edge in edges:
                if edge.key in consumed_edges:
                    continue

                move_pen(edge, edge.vertices[1] if edge.vertices[0] == pen.vertex_index else edge.vertices[0])
                return edge

        next_index = -1
        distance = 0
        next_edge: bpy.types.MeshEdge | None = None

        pen_location = pen.location()
        
        for edge in remaining_edges:
            if edge.key in consumed_edges:
                continue

            for vertex in edge.vertices:
                curr_distance = (vertices[vertex] - pen_location).magnitude

                if next_index == -1 or curr_distance < distance:
                    next_index = vertex
                    distance = curr_distance
                    next_edge = edge

        if next_edge == None:
            raise Exception('could not find an edge')
        
        move_pen(next_edge, other_vertex(next_edge, next_index))

        return next_edge

    edge_order: list[bpy.types.MeshEdge] = []

    while len(remaining_edges) > 0:
        next_edge = find_next_edge()
        edge_order.append(next_edge)

    def get_joint_point(edge, vertex_index, is_forward):
        edges = vertex_to_edge.get(vertex_index)

        if not edges:
            raise Exception('could nto find vertex')

        index = edges.index(edge)

        vertex_joints = joints.get(vertex_index)

        if not vertex_joints:
            raise Exception('could not find vertex_joints')
            
        if is_forward:
            return vertex_joints[index][0]
        
        joints_for_edge = vertex_joints[(index - 1 + len(vertex_joints)) % len(vertex_joints)]

        return joints_for_edge[-1]
        
    current_distance = 0
    total_distance = 0

    for edge in edge_order:
        a = edge.vertices[0]
        b = edge.vertices[1]

        total_distance += (vertices[a] - vertices[b]).magnitude

    current_vertex = pen.first_vertex
    uv_scale = 1 / total_distance

    for edge in edge_order:
        index_start = len(into.vertices)

        a = edge.vertices[0]
        b = edge.vertices[1]

        next_distance = current_distance + (vertices[a] - vertices[b]).magnitude

        if a == current_vertex:
            a_uv = current_distance * uv_scale
            b_uv = next_distance * uv_scale
            current_vertex = b
        else:
            a_uv = next_distance * uv_scale
            b_uv = current_distance * uv_scale
            current_vertex = a

        into.append_vertex(create_vertex(vertices[a], 0, a_uv))
        into.append_vertex(create_vertex(vertices[b], 0, b_uv))
        into.append_vertex(create_vertex(get_joint_point(edge, a, False), 0.5, a_uv))
        into.append_vertex(create_vertex(get_joint_point(edge, b, True), 0.5, b_uv))
        into.append_vertex(create_vertex(get_joint_point(edge, b, False), 0.5, b_uv))
        into.append_vertex(create_vertex(get_joint_point(edge, a, True), 0.5, a_uv))

        into.append_triangle(index_start+0, index_start+2, index_start+1)
        into.append_triangle(index_start+2, index_start+3, index_start+1)
        into.append_triangle(index_start+1, index_start+4, index_start+5)
        into.append_triangle(index_start+1, index_start+5, index_start+0)

        current_distance = next_distance

class MapRoom():
    def __init__(self, room_index: int):
        self.room_index: int = room_index
        self.mesh: mesh.mesh_data = mesh.mesh_data(None)

def build_map_outline(outlines: list[MapEntry], file):
    if len(outlines) == 0:
        file.write(struct.pack('>Hffff', 0, 0, 0, 0, 0)) 
        return
    
    outlines = sorted(outlines, key=lambda x: x.room_index)
    
    min_pos, max_pos = outlines[0].bounding_box()

    for entry in outlines:
        min_pos, max_pos = bounding_box.union((min_pos, max_pos), entry.bounding_box())

    size = max_pos - min_pos

    max_size = max(size.x, size.y)

    global_transform = mathutils.Matrix.Scale(TOTAL_SCALE / max_size, 4) @ mathutils.Matrix.Translation(-min_pos)    

    current_mesh = MapRoom(outlines[0].room_index)
    result = [current_mesh]

    for outline in outlines:
        if current_mesh.room_index != outline.room_index:
            result.append(MapRoom(outline.room_index))

        write_outline(current_mesh.mesh, outline.obj, global_transform)

    file.write(struct.pack('>Hffff', len(result), min_pos.x, min_pos.y, max_pos.x, max_pos.y))

    settings = export_settings.ExportSettings()
    settings.fixed_point_scale = 1
    settings.world_scale = 1   

    for room in result:
        file.write(struct.pack('>B', 0))
        tiny3d_mesh_writer.write_mesh([room.mesh], None, [], settings, file)


    

    



