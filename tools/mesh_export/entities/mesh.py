import bpy
import mathutils
from . import material
from . import serialize
import os
import struct
import bmesh
import sys
import math

from . import armature
from . import material_extract

class mesh_data():
    def __init__(self, mat: bpy.types.Material) -> None:
        self.mat: bpy.types.Material = mat
        self.vertices = []
        self.normals = []
        self.color = []
        self.uv = []
        self.indices = []
        self.bone_indices = []

    def copy(self):
        result = mesh_data(self.mat)

        result.vertices = self.vertices.copy()
        result.normals = self.normals.copy()
        result.color = self.color.copy()
        result.uv = self.uv.copy()
        result.indices = self.indices.copy()
        result.bone_indices = self.bone_indices.copy()

        return result

    def append_mesh(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, material_index: int, final_transform: mathutils.Matrix, armature: armature.ArmatureData | None):
        triangles = []
        max_index = -1
        used_indices = set()

        normal_transform = final_transform.to_3x3()
        normal_transform.invert()
        normal_transform.transpose()
        bone_index = -1

        if obj.parent_bone and armature:
            bone = armature.find_bone_data(obj.parent_bone)
            final_transform = bone.pose_bone.matrix.inverted() @ obj.matrix_world
            normal_transform = final_transform.to_3x3().inverted().transposed()
            bone_index = bone.index
            
            # world = parent_inverse @ obj.matrix_world

        for polygon in mesh.polygons:
            if polygon.material_index != material_index:
                continue
            
            for loop_index in polygon.loop_indices:
                max_index = max(max_index, loop_index)
                used_indices.add(loop_index)
                
            triangles.append(polygon.loop_indices[0])
            triangles.append(polygon.loop_indices[1])
            triangles.append(polygon.loop_indices[2])

        next_output = len(self.indices)
        index_mapping = dict()

        uv_layer = None
        
        for layer in mesh.uv_layers:
            if layer.active:
                uv_layer = layer
                break

        color = None

        for attr in mesh.attributes:
            if attr.data_type == 'BYTE_COLOR' or attr.data_type == 'FLOAT_COLOR':
                color = attr

        for loop_index in range(max_index + 1):
            if not loop_index in used_indices:
                continue

            vtx_index = mesh.loops[loop_index].vertex_index

            vertex_transform = final_transform
            normal_vertex_transform = normal_transform

            bone_name = None
            group_index = None
            bone = None

            if not armature is None and not obj.parent_bone:
                group_index = -1
                group_weight = 0

                for element in mesh.vertices[vtx_index].groups:
                    if element.weight > group_weight:
                        group_weight = element.weight
                        group_index = element.group

                if group_index != -1:
                    bone_name = obj.vertex_groups[group_index].name
                    bone = armature.find_bone_data(bone_name)
                    bone_index = bone.index

                    vertex_transform = bone.matrix_scene_inv @ final_transform
                    normal_vertex_transform = bone.matrix_normal_inv @ normal_transform

            pos = vertex_transform @ mesh.vertices[vtx_index].co

            # if vtx_index == 79:
            #     print(bone_name, vtx_index, mesh.vertices[vtx_index].co, pos)


            self.vertices.append(pos)
            self.normals.append(normal_vertex_transform @ mesh.vertices[vtx_index].normal)

            if uv_layer:
                self.uv.append(uv_layer.uv[loop_index].vector)
            else:
                self.uv.append([0, 0])

            if color and color.domain == 'CORNER':
                self.color.append(color.data[loop_index].color)
            elif color and color.domain == 'POINT':
                self.color.append(color.data[vtx_index].color)
            else:
                self.color.append([1, 1, 1, 1])

            self.bone_indices.append(bone_index)

            index_mapping[loop_index] = next_output
            next_output += 1
        
        for idx in triangles:
            self.indices.append(index_mapping[idx])

    
def convert_vertex_channel(input, gamma):
    result = round(255 * math.pow(input, gamma))

    if result > 255:
        return 255
    elif result < 0:
        return 0
    
    return result

def pack_vertex(vertex, uv, color, normal, bone_index, gamma = 1):
    result = struct.pack(
        ">hhh", 
        round(vertex[0] * 64), 
        round(vertex[1] * 64), 
        round(vertex[2] * 64)
    )

    if uv:
        result = result + struct.pack(
            ">hh",
            round(uv[0] * 256),
            round((1 - uv[1]) * 256)
        )

    if color:
        result = result + struct.pack(
            ">BBBB", 
            convert_vertex_channel(color[0], gamma), 
            convert_vertex_channel(color[1], gamma), 
            convert_vertex_channel(color[2], gamma), 
            convert_vertex_channel(color[3], 1)
        )

    if normal:
        normal = normal.normalized()
        result = result + struct.pack(
            ">bbb", 
            round(normal[0] * 127), 
            round(normal[1] * 127), 
            round(normal[2] * 127)
        )

    if bone_index != None:
        result = result + struct.pack(
            ">h", 
            bone_index
        )

    return result

ATTR_POS = 1 << 0
ATTR_UV = 1 << 1
ATTR_COLOR = 1 << 2
ATTR_NORMAL = 1 << 3
ATTR_MATRIX = 1 << 4

def _write_meshes(file, mesh_list, armature: armature.ArmatureData):
    file.write('MESH'.encode())
    file.write(len(mesh_list).to_bytes(1, 'big'))

    for mesh_pair in mesh_list:
        material_name = mesh_pair[0]
        mesh: mesh_data = mesh_pair[1]

        material_filename = f"assets/{material_name}.mat.json"
        material_object = material_extract.load_material_with_name(material_name, mesh.mat)

        if not material_name.startswith('materials/'):
            print(f"embedding material {material_name}")

            # signal an embedded material
            file.write((0).to_bytes(1, 'big'))

            serialize.serialize_material_file(file, material_object)

        elif os.path.exists(material_filename):
            material_romname = f"rom:/{material_name}.mat".encode()
            file.write(len(material_romname).to_bytes(1, 'big'))
            file.write(material_romname)
        else:
            raise Exception(f"{material_filename} does not exist")
    
        needs_uv = bool(material_object.tex0)
        needs_normal = bool(material_object.lighting)
        needs_color = len(mesh.color) > 0

        attribute_mask = ATTR_POS

        if needs_uv:
            attribute_mask |= ATTR_UV

        if needs_color:
            attribute_mask |= ATTR_COLOR

        if needs_normal:
            attribute_mask |= ATTR_NORMAL

        if armature:
            attribute_mask |= ATTR_MATRIX

        file.write((attribute_mask).to_bytes(1, 'big'))

        # TODO deduplicate vertices
        # TODO optimize triangle order

        file.write(len(mesh.vertices).to_bytes(2, 'big'))
        for idx, vertex in enumerate(mesh.vertices):
            file.write(pack_vertex(
                vertex,
                mesh.uv[idx] if needs_uv else None,
                mesh.color[idx] if needs_color else None,
                mesh.normals[idx] if needs_normal else None,
                mesh.bone_indices[idx] if armature else None,

                gamma=material_object.vertex_gamma,
            ))

        index_size = 1

        if len(mesh.vertices) > 256:
            index_size = 2

        file.write(len(mesh.indices).to_bytes(2, 'big'))

        for index in mesh.indices:
            file.write(index.to_bytes(index_size, 'big'))

def _pack_position(input: float) -> int:
    return round(input * 256)

def _write_packed_quaternion(file, input: mathutils.Quaternion):
    if input.w < 0:
        final_input = -input
    else:
        final_input = input

    file.write(struct.pack(
        ">hhh",
        round(32767 * final_input.x),
        round(32767 * final_input.y),
        round(32767 * final_input.z)
    ))

class mesh_list_entry:
    def __init__(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.obj: bpy.types.Object= obj
        self.mesh: bpy.types.Mesh = mesh
        self.transform: mathutils.Matrix = transform

class mesh_list():
    def __init__(self, base_transform: mathutils.Matrix) -> None:
        self.meshes: list[mesh_list_entry] = []
        self.base_transform: mathutils.Matrix = base_transform

    def append(self, obj):
        mesh = obj.data
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()
        self.meshes.append(mesh_list_entry(obj, mesh, self.base_transform @ obj.matrix_world))

    def determine_mesh_data(self, armature: armature.ArmatureData | None = None):
        mesh_by_material: dict[str, mesh_data] = dict()

        for entry in self.meshes:
            mesh = entry.mesh
            transform = entry.transform

            for material_index in range(max(len(mesh.materials), 1)):
                if material_index < len(mesh.materials):
                    mat = mesh.materials[material_index]
                    name = mat.name
                else:
                    mat = None
                    name = ""

                if not name in mesh_by_material:
                    mesh_by_material[name] = mesh_data(mat)

                mesh_data_instance = mesh_by_material[name]

                mesh_data_instance.append_mesh(entry.obj, mesh, material_index, transform, armature)

        all_meshes = list(mesh_by_material.items())

        all_meshes.sort(key=lambda x: x[0])

        return all_meshes


    def write_mesh(self, write_to, armature: armature.ArmatureData | None = None):
        all_meshes = self.determine_mesh_data(armature)
        _write_meshes(write_to, all_meshes, armature)
        armature.write_armature(write_to, armature)
