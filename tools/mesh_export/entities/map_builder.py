import bpy
import mathutils
import collections
import math
import struct

from . import bounding_box
from . import mesh
from . import tiny3d_mesh_writer
from . import export_settings
from . import mesh2d_writer

TARGET_MAP_SIZE = 200

TOTAL_SCALE = TARGET_MAP_SIZE

LINE_WIDTH = 2

class MapEntry():
    def __init__(self, obj: bpy.types.Object, room_index: int):
        self.obj: bpy.types.Object = obj
        self.room_index: int = room_index

    def bounding_box(self) -> tuple[mathutils.Vector, mathutils.Vector]:
        return bounding_box.from_obj(self.obj)
    
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


def write_outline(into: mesh2d_writer.Mesh2d, obj: bpy.types.Object, global_transform: mathutils.Matrix):
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

    for vertex_index, edges in vertex_to_edge.items():
        pivot = vertices[vertex_index]

        def other_point(edge: bpy.types.MeshEdge) -> mathutils.Vector:
            return vertices[other_vertex(edge, vertex_index)]

        def edge_angle(edge: bpy.types.MeshEdge) -> float:
            dir = other_point(edge) - pivot
            return math.atan2(dir.y, dir.x)

        edges.sort(key=edge_angle)

    consumed_edges = set()

    pen = Pen(vertices)
    actions: list[tuple[int, bool]] = []

    def move_pen(edge: bpy.types.MeshEdge, next_index: int, is_up: bool):
        pen.move_to(next_index)
        actions.append((next_index, is_up))
        remaining_edges.remove(edge)
        consumed_edges.add(edge.key)

    def find_next_edge() -> bpy.types.MeshEdge:
        if pen.vertex_index != -1:
            edges = vertex_to_edge[pen.vertex_index]

            for edge in edges:
                if edge.key in consumed_edges:
                    continue

                move_pen(edge, edge.vertices[1] if edge.vertices[0] == pen.vertex_index else edge.vertices[0], False)
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
        
        actions.append((next_index, True))
        move_pen(next_edge, other_vertex(next_edge, next_index), False)

        return next_edge

    while len(remaining_edges) > 0:
        find_next_edge()
        
    current_distance = 0
    total_distance = 0

    pos: mathutils.Vector = mathutils.Vector()

    for action in actions:
        vertex_index, is_up = action

        if not is_up:
            total_distance += (vertices[vertex_index] - pos).magnitude
        
        pos = vertices[vertex_index]

    uv_scale = 1 / total_distance
    
    pos: mathutils.Vector = mathutils.Vector()

    for action in actions:
        vertex_index, is_up = action

        current_pos = vertices[vertex_index]
        uv = current_distance * uv_scale

        if is_up:
            into.commands.append(mesh2d_writer.Mesh2DMoveTo(current_pos, uv, 2, [1, 1, 1, 1]))
        else:
            into.commands.append(mesh2d_writer.Mesh2DLineTo(current_pos, uv, 2, [1, 1, 1, 1]))

        if not is_up:
            current_distance += (vertices[vertex_index] - pos).magnitude

        pos = current_pos

class MapRoom():
    def __init__(self, room_index: int):
        self.room_index: int = room_index
        self.mesh: mesh2d_writer.Mesh2d = mesh2d_writer.Mesh2d()

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
        room.mesh.write(file)


    

    



