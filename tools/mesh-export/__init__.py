import bpy
import mathutils
import bmesh
import sys
import struct

class mesh_data():
    def __init__(self) -> None:
        self.vertices = []
        self.normals = []
        self.indices = []

    def append_mesh(self, mesh: bpy.types.Mesh, material_index: int):
        triangles = []
        max_index = -1
        used_indices = set()

        for polygon in mesh.polygons:
            if polygon.material_index != material_index:
                continue
            
            for loop_index in polygon.loop_indices:
                idx = mesh.loops[loop_index].vertex_index

                max_index = max(max_index, idx)
                used_indices.add(idx)
                triangles.append(idx)

        next_output = len(self.indices)
        index_mapping = dict()

        for input_idx in range(max_index + 1):
            if not input_idx in used_indices:
                continue

            self.vertices.append(mesh.vertices[input_idx].co)
            self.normals.append(mesh.vertices[input_idx].normal)

            index_mapping[input_idx] = next_output
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
            # hard code a 9 to indicate position and normal data
            # data
            file.write((1 | 8).to_bytes(2, 'big'))

            file.write(len(mesh[1].vertices).to_bytes(2, 'big'))
            for idx, vertex in enumerate(mesh[1].vertices):
                file.write(struct.pack(
                    ">hhh", 
                    int(vertex[0] * 32), 
                    int(vertex[1] * 32), 
                    int(vertex[2] * 32)
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

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        for material_index in range(max(len(obj.data.materials), 1)):
            if material_index < len(obj.data.materials):
                name = obj.data.materials[material_index].name
            else:
                name = ""

            if not name in mesh_by_material:
                mesh_by_material[name] = mesh_data()

            mesh = mesh_by_material[name]

            mesh.append_mesh(obj.data, material_index)

    all_meshes = list(mesh_by_material.items())

    all_meshes.sort(key=lambda x: x[0])

    write_meshes(sys.argv[-1], all_meshes)

process_scene()