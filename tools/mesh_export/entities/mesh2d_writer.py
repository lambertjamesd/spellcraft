import struct
import mathutils
import typing
import math

from . import mesh

MESH2D_CMD_MATERIAL = 0
MESH2D_CMD_MOVE_TO = 1
MESH2D_CMD_LINE_TO = 2

def pack_vertex(file, pos: mathutils.Vector, u: float, w: float, color):
    file.write(struct.pack(
        '>hhhHBBBB',
        math.floor(pos.x * 4 + 0.5),
        math.floor(pos.y * 4 + 0.5),
        math.floor(u * 256),
        math.floor(w * 4 + 0.5),
        mesh.convert_vertex_channel(color[0], 0.454545),
        mesh.convert_vertex_channel(color[1], 0.454545),
        mesh.convert_vertex_channel(color[2], 0.454545),
        mesh.convert_vertex_channel(color[3], 1)
    ))

class Mesh2DMaterial():
    def __init__(self, index: int):
        self.index: int = index

    def write(self, file):
        file.write(struct.pack('>BH', MESH2D_CMD_MATERIAL, self.index))
        
class Mesh2DMoveTo():
    def __init__(self, pos: mathutils.Vector, u: float, w: float, color):
        self.pos: mathutils.Vector = pos
        self.u: float = u
        self.w: float = w
        self.color = color
        
    def write(self, file):
        file.write(struct.pack('>B', MESH2D_CMD_MOVE_TO))
        pack_vertex(file, self.pos, self.u, self.w, self.color)
        
class Mesh2DLineTo():
    def __init__(self, pos: mathutils.Vector, u: float, w: float, color):
        self.pos: mathutils.Vector = pos
        self.u: float = u
        self.w: float = w
        self.color = color

    def write(self, file):
        file.write(struct.pack('>B', MESH2D_CMD_LINE_TO))
        pack_vertex(file, self.pos, self.u, self.w, self.color)

Mesh2DCommand = typing.Union[Mesh2DMaterial, Mesh2DMoveTo, Mesh2DLineTo]

class Mesh2d:
    def __init__(self, materialName: str | None = None):
        self.materialName: str | None = materialName
        self.commands: list[Mesh2DCommand] = []

    def write(self, file):
        file.write('MSH2'.encode())
        # transition count
        file.write(struct.pack('>HH', 0, len(self.commands)))

        for command in self.commands:
            command.write(file)