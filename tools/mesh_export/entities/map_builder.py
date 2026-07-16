import bpy
import mathutils
import collections
import math
import struct
import io

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
        self.layer: int = 0

        if hasattr(obj, 'map_layer'):
            self.layer = obj.map_layer

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

    uv_scale = 32 / total_distance
    
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

class MapLayerRoom():
    def __init__(self, room_index: int, layer_index: int):
        self.room_index: int = room_index
        self.layer_index: int = layer_index
        self.mesh: mesh2d_writer.Mesh2d = mesh2d_writer.Mesh2d()
        self.max_y: float | None = None

    def append(self, obj: bpy.types.Object, global_transform: mathutils.Matrix):
        write_outline(self.mesh, obj, global_transform)

        if self.max_y == None or obj.location.z > self.max_y:
            self.max_y = obj.location.z

    def write(self, file: io.BufferedIOBase):
        file.write(struct.pack('>HH', self.room_index, 0))
        self.mesh.write(file)


class MapLayer():
    def __init__(self, index: int):
        self.rooms: list[MapLayerRoom] = []
        self.index: int = index

    def get_room(self, room_index: int) -> MapLayerRoom:
        for room in self.rooms:
            if room.room_index == room_index:
                return room

        result = MapLayerRoom(room_index, self.index)
        self.rooms.append(result)
        return result
    
    def write(self, file: io.BufferedIOBase):
        file.write(struct.pack('>H', len(self.rooms)))

        for room in self.rooms:
            room.write(file)

def _layer_range(rooms: list[MapLayerRoom]) -> tuple[int, int]:
    if len(rooms) == 0:
        return 0, 0

    min_layer = rooms[0].layer_index
    layer_count = rooms[-1].layer_index - min_layer + 1
    return min_layer, layer_count

    
class Map():
    def __init__(self):
        self.layers: list[MapLayer] = []

    def get_room(self, room_index: int, layer_index: int):
        while len(self.layers) <= layer_index:
            self.layers.append(MapLayer(len(self.layers)))

        return self.layers[layer_index].get_room(room_index)
    
    def write(self, min_pos: mathutils.Vector, max_pos: mathutils.Vector, file: io.BufferedIOBase):
        room_lookup: list[list[MapLayerRoom]] = []

        for layer in self.layers:
            for room in layer.rooms:
                while len(room_lookup) <= room.room_index:
                    room_lookup.append([])

                room_lookup[room.room_index].append(room)

        layer_room_count = sum(map(lambda x: len(x.rooms), self.layers))
        room_layer_y_count = sum(map(lambda x: _layer_range(x)[1], room_lookup))

        file.write(struct.pack(
            '>HHHHffff', 
            len(self.layers), 
            len(room_lookup),
            layer_room_count,
            room_layer_y_count,
            min_pos.x, 
            -max_pos.y, 
            max_pos.x, 
            -min_pos.y
        ))

        for layer in self.layers:
            layer.write(file)

        for room in room_lookup:
            sorted_by_layer = sorted(room, key = lambda x: x.layer_index)

            if len(sorted_by_layer) == 0:
                file.write(struct.pack('>HH', 0, 0))
                continue

            min_layer, layer_count = _layer_range(sorted_by_layer)

            file.write(struct.pack('>HH', min_layer, layer_count))

            curr_index = 0
            curr_y = sorted_by_layer[0].max_y

            for layer_index in range(min_layer, min_layer+layer_count):
                if curr_index < len(sorted_by_layer) and sorted_by_layer[curr_index].layer_index == layer_index:
                    curr_y = sorted_by_layer[curr_index].max_y
                    curr_index += 1

                file.write(struct.pack('>f', curr_y))

HEADER_FOOTER = 'MAP '.encode()

def build_map_outline(outlines: list[MapEntry], file):
    file.write(HEADER_FOOTER)

    if len(outlines) == 0:
        file.write(struct.pack('>HHHHffff', 0, 0, 0, 0, 0, 0, 0, 0)) 
        file.write(HEADER_FOOTER)
        return
    
    outlines = sorted(outlines, key=lambda x: x.room_index)
    
    min_pos, max_pos = outlines[0].bounding_box()

    for entry in outlines:
        min_pos, max_pos = bounding_box.union((min_pos, max_pos), entry.bounding_box())

    size = max_pos - min_pos

    max_size = max(size.x, size.y)

    center_offset = mathutils.Vector((max_size - size.x, max_size - size.y, 0)) * 0.5

    min_pos = min_pos - center_offset
    max_pos = max_pos - center_offset

    global_transform = mathutils.Matrix((
        (-1, 0, 0, TOTAL_SCALE),
        (0, 1, 0, 0),
        (0, 0, 1, 0),
        (0, 0, 0, 1)
    )) @ mathutils.Matrix.Scale(TOTAL_SCALE / max_size, 4) @ mathutils.Matrix.Translation(-min_pos)    
    
    map = Map()

    for outline in outlines:
        current_mesh = map.get_room(outline.room_index, outline.layer)
        current_mesh.append(outline.obj, global_transform)

    map.write(min_pos, max_pos, file)
    file.write(HEADER_FOOTER)


    

    



