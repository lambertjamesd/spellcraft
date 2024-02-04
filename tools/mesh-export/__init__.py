import bpy
import mathutils
import bmesh
import sys
import struct
import math

class mesh_data():
    def __init__(self) -> None:
        self.vertices = []
        self.normals = []
        self.indices = []
        self.uv = []

    def append_mesh(self, mesh: bpy.types.Mesh, material_index: int, final_transform: mathutils.Matrix):
        triangles = []
        max_index = -1
        used_indices = set()

        normal_transform = final_transform.to_3x3()
        normal_transform.invert()
        normal_transform.transpose()

        for polygon in mesh.polygons:
            if polygon.material_index != material_index:
                continue
            
            for loop_index in polygon.loop_indices:
                max_index = max(max_index, loop_index)
                used_indices.add(loop_index)
                triangles.append(loop_index)

        next_output = len(self.indices)
        index_mapping = dict()

        uv_layer = None if len(mesh.uv_layers) == 0 else mesh.uv_layers[0]

        for loop_index in range(max_index + 1):
            if not loop_index in used_indices:
                continue

            vtx_index = mesh.loops[loop_index].vertex_index

            self.vertices.append(final_transform @ mesh.vertices[vtx_index].co)
            self.normals.append(normal_transform @ mesh.vertices[vtx_index].normal)

            if uv_layer:
                self.uv.append(mesh.uv_layers[0].uv[loop_index].vector)
            else:
                self.uv.append([0, 0])

            index_mapping[loop_index] = next_output
            next_output += 1
        
        for idx in triangles:
            self.indices.append(index_mapping[idx])

mesh_by_material = dict()

def write_meshes(filename, mesh_list):
    with open(filename, "wb") as file:
        file.write('MESH'.encode())
        file.write(len(mesh_list).to_bytes(2, 'big'))

        for mesh in mesh_list:
            material_name = mesh[0].encode()
            file.write(len(material_name).to_bytes(2, 'big'))
            file.write(material_name)

            # indicate which attributes are present
            # hard code a 11 to indicate position, uv, and normal data
            # data
            file.write((1 | 2 | 8).to_bytes(2, 'big'))

            file.write(len(mesh[1].vertices).to_bytes(2, 'big'))
            for idx, vertex in enumerate(mesh[1].vertices):
                file.write(struct.pack(
                    ">hhh", 
                    int(vertex[0] * 32), 
                    int(vertex[1] * 32), 
                    int(vertex[2] * 32)
                ))

                uv = mesh[1].uv[idx]

                file.write(struct.pack(
                    ">hh",
                    int(uv[0] * 256),
                    int(uv[1] * 256)
                ))

                normal = mesh[1].normals[idx]

                file.write(struct.pack(
                    ">bbb", 
                    int(normal[0] * 127), 
                    int(normal[1] * 127), 
                    int(normal[2] * 127)
                ))

            index_size = 1

            if len(mesh[1].vertices) > 256:
                index_size = 2

            file.write(len(mesh[1].indices).to_bytes(2, 'big'))

            for index in mesh[1].indices:
                file.write(index.to_bytes(index_size, 'big'))



def process_scene():
    bpy.ops.object.mode_set(mode="OBJECT")

    for mesh in bpy.data.meshes:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        final_transform = base_transform @ obj.matrix_world

        for material_index in range(max(len(obj.data.materials), 1)):
            if material_index < len(obj.data.materials):
                name = obj.data.materials[material_index].name
            else:
                name = ""

            if not name in mesh_by_material:
                mesh_by_material[name] = mesh_data()

            mesh = mesh_by_material[name]

            mesh.append_mesh(obj.data, material_index, final_transform)

    all_meshes = list(mesh_by_material.items())

    all_meshes.sort(key=lambda x: x[0])

    write_meshes(sys.argv[-1], all_meshes)

process_scene()