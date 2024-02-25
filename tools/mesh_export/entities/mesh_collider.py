import bpy
import mathutils
import bmesh
import struct

class MeshColliderTriangle():
    def __init__(self, indices: list[float]):
        self.indices: list[float] = indices

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