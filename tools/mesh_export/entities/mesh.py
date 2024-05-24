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
        bone_index = 0

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

                    vertex_transform = bone.matrix_world_inv @ final_transform
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

def search_node_input(from_node: bpy.types.Node, input_name: str):
    for input in from_node.inputs:
        if input.name == input_name:
            return input
        
    return None


def search_node_linkage(from_node: bpy.types.Node, input_name: str):
    input = search_node_input(from_node, input_name)

    if not input.is_linked:
        return None

    for link in input.links:
        return link.from_node
        
    return None

def find_node_of_type(tree_nodes: bpy.types.Nodes, type_name: str):
    for node in tree_nodes:
        if node.type == type_name:
            return node
        
    return None

def color_float_to_int(value):
    result = round(value * 255)

    if result > 255:
        result = 255
    elif result < 0:
        result = 0

    return result

def color_array_to_color(array):
    return material.Color(
        color_float_to_int(array[0]),
        color_float_to_int(array[1]),
        color_float_to_int(array[2]),
        color_float_to_int(array[3])
    )

def determine_material_from_nodes(mat: bpy.types.Material, result: material.Material):
    output_node = find_node_of_type(mat.node_tree.nodes, 'OUTPUT_MATERIAL')

    if not output_node:
        print('Could not find output node for material')
        return

    material_type = search_node_linkage(output_node, 'Surface')

    if not material_type:
        print('No input was specified for output material')
        return

    color_link: bpy.types.NodeSocket | None = None

    if material_type.type == 'BSDF_PRINCIPLED':
        color_link = search_node_input(material_type, 'Base Color')
        emmission_link = search_node_input(material_type, 'Emission Color')
        result.lighting = True

        if not color_link.is_linked and emmission_link.is_linked:
            color_link = emmission_link
            result.lighting = False

    elif material_type.type == 'BSDF_DIFFUSE':
        color_link = search_node_input(material_type, 'Color')
        result.lighting = True
    elif material_type.type == 'EMISSION':
        color_link = search_node_input(material_type, 'Color')
        result.lighting = False
    elif material_type.type == 'MIX':
        color_link = search_node_input(output_node, 'Surface')
        result.lighting = False
    else:
        print(f"The node type {material_type.type} is not supported")
        return

    if not color_link:
        print('couldnt find color link')
        return
    
    # TODO determine alpha

    color_name = 'PRIM'
    
    # solid color
    if not color_link.is_linked:
        color = color_link.default_value
        result.prim_color = color_array_to_color(color)
        color_name = 'PRIM'
    elif color_link.links[0].from_node.type == 'TEX_IMAGE':
        color_node: bpy.types.ShaderNodeTexImage = color_link.links[0].from_node
        color_name = 'TEX0'

        input_filename = sys.argv[1]
        image_path = os.path.normpath(os.path.join(os.path.dirname(input_filename), color_node.image.filepath[2:]))

        result.tex0 = material.Tex()
        result.tex0.filename = image_path

        if color_node.interpolation == 'Nearest':
            result.tex0.min_filter = 'nearest'
            result.tex0.mag_filter = 'nearest'
        else:
            result.tex0.min_filter = 'linear'
            result.tex0.mag_filter = 'linear'

        if color_node.extension == 'REPEAT':
            result.tex0.s.repeats = 2048
            result.tex0.s.mirror = False
            result.tex0.t.repeats = 2048
            result.tex0.t.mirror = False
        elif color_node.extension == 'MIRROR':
            result.tex0.s.repeats = 2048
            result.tex0.s.mirror = True
            result.tex0.t.repeats = 2048
            result.tex0.t.mirror = True
        else:
            result.tex0.s.repeats = 1
            result.tex0.s.mirror = False
            result.tex0.t.repeats = 1
            result.tex0.t.mirror = False

    if result.lighting:
        result.combine_mode = material.CombineMode(
            material.CombineModeCycle(
                color_name, '0', 'SHADE', '0',
                color_name, '0', 'SHADE', '0'
            ),
            None
        )
    else:
        result.combine_mode = material.CombineMode(
            material.CombineModeCycle(
                '0', '0', '0', color_name,
                '0', '0', '0', color_name
            ),
            None
        )

    if mat.use_backface_culling:
        result.culling = True
    else:
        result.culling = False

    if mat.blend_method == 'CLIP':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'THRESHOLD'
        result.blend_color = material.Color(0, 0, 0, 128)
    elif mat.blend_method == 'BLEND':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'), None)
        result.blend_mode.z_write = False
    elif mat.blend_method == 'HASHED':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'NOISE'
    else:
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)

    if 'decal' in mat and mat['decal']:
        result.blend_mode.z_mode = 'DECAL'
        result.blend_mode.z_write = False

    
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
        round(vertex[0] * 32), 
        round(vertex[1] * 32), 
        round(vertex[2] * 32)
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
        result = result + struct.pack(
            ">bbb", 
            round(normal[0] * 127), 
            round(normal[1] * 127), 
            round(normal[2] * 127)
        )

    if bone_index != None:
        result = result + bone_index.to_bytes(1, 'big')

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

        if not material_name.startswith('materials/'):
            # embedded material
            material_object = material.Material()

            if mesh.mat.use_nodes:
                determine_material_from_nodes(mesh.mat, material_object)

            print(f"embedding material {material_name}")

            # signal an embedded material
            file.write((0).to_bytes(1, 'big'))

            serialize.serialize_material_file(file, material_object)

        elif os.path.exists(material_filename):
            print(f"using existing material {material_filename}")
            material_object = material.parse_material(material_filename)

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

def _write_armature(file, arm: armature.ArmatureData | None):
    file.write('ARMT'.encode())
    
    if arm is None:
        file.write((0).to_bytes(2, 'big'))
        return
    
    bones = arm.get_filtered_bones()
    
    file.write(len(bones).to_bytes(2, 'big'))

    for bone in bones:
        parent = arm.find_parent_bone(bone.name)

        if parent:
            file.write(parent.index.to_bytes(1, 'big'))
        else:
            file.write((255).to_bytes(1, 'big'))

    default_pose = arm.generate_pose_data()

    for bone_pose in default_pose:
        bone_pose.write_to_file(file)

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

    def write_mesh(self, write_to, armature: armature.ArmatureData | None = None):
        mesh_by_material = dict()

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

        _write_meshes(write_to, all_meshes, armature)
        _write_armature(write_to, armature)
