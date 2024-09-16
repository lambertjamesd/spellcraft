import bpy
import os.path
import sys

from . import material
from . import serialize

def _reverse_lookup(mapping: dict, value):
    for entry in mapping.items():
        if entry[1] == value:
            return entry[0]

    raise Exception(f'Could not find value {value} in reverse lookup')

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

def _determine_combiner_from_f3d(combiner) -> material.CombineModeCycle:
     return material.CombineModeCycle(
        _reverse_lookup(serialize.COMB_A, combiner['A']),
        _reverse_lookup(serialize.COMB_B, combiner['B']),
        _reverse_lookup(serialize.COMB_C, combiner['C']),
        _reverse_lookup(serialize.COMB_D, combiner['D']),
        _reverse_lookup(serialize.ACOMB_A, combiner['A_alpha']),
        _reverse_lookup(serialize.ACOMB_A, combiner['B_alpha']),
        _reverse_lookup(serialize.ACOMB_MUL, combiner['C_alpha']),
        _reverse_lookup(serialize.ACOMB_A, combiner['D_alpha']),
    )

def _determine_blend_from_f3d(rdp_settings, index) -> material.BlendModeCycle:
    return material.BlendModeCycle(
        _reverse_lookup(serialize.BLEND_A,rdp_settings[f'blend_p{index}']),
        _reverse_lookup(serialize.BLEND_B1,rdp_settings[f'blend_a{index}']),
        _reverse_lookup(serialize.BLEND_A,rdp_settings[f'blend_m{index}']),
        _reverse_lookup(serialize.BLEND_B1,rdp_settings[f'blend_b{index}']),
    )

def _determine_color_from_f3d(color) -> material.Color:
    return material.Color(
        int(color[0] * 255),
        int(color[1] * 255),
        int(color[2] * 255),
        int(color[3] * 255)
    )

def _determine_tex_axis_from_f3d(axis, result: material.TexAxis):
    result.scale_log = axis['shift']

    if 'clamp' in axis and axis['clamp']:
        result.repeats = 0
    
    if 'mirror' in axis and axis['mirror']:
        result.mirror = True


def _determine_tex_from_f3d(tex) -> material.Tex:
    image = tex['tex']

    if not tex['tex_set'] or not image:
        return None

    input_filename = sys.argv[1]

    result = material.Tex()
    result.filename = os.path.normpath(os.path.join(os.path.dirname(input_filename), image.filepath[2:]))
    _determine_tex_axis_from_f3d(tex['S'], result.s)
    _determine_tex_axis_from_f3d(tex['T'], result.t)

    return result


def determine_material_from_f3d(mat: bpy.types.Material) -> material.Material:
    rdp_settings = mat['f3d_mat']['rdp_settings']

    is_2_cycle = rdp_settings['g_mdsft_cycletype'] == 1
    
    result: material.Material = material.Material()
    result.combine_mode = material.CombineMode(
        _determine_combiner_from_f3d(mat['f3d_mat']['combiner1']),
        _determine_combiner_from_f3d(mat['f3d_mat']['combiner2']) if is_2_cycle else None,
    )

    draw_layer = mat['f3d_mat']['draw_layer']

    if draw_layer == 0 or draw_layer == 1:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'OPAQUE'
    elif draw_layer == 2:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'DECAL'
    elif draw_layer == 3:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'INTER'
    elif draw_layer == 4:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'OPAQUE'
    elif draw_layer == 5 or draw_layer == 6 or draw_layer == 7:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "IN_A", "MEMORY", "INV_MUX_A"), None)
        result.blend_mode.z_mode = 'TRANSPARENT'
        result.blend_mode.z_write = False

    if mat['f3d_mat']['set_env']:
        result.env_color = _determine_color_from_f3d(mat['f3d_mat']['env_color'])

    if mat['f3d_mat']['set_prim']:
        result.env_color = _determine_color_from_f3d(mat['f3d_mat']['prim_color'])

    if mat['f3d_mat']['set_blend']:
        result.env_color = _determine_color_from_f3d(mat['f3d_mat']['blend_color'])

    if mat['f3d_mat']['set_prim']:
        result.env_color = _determine_color_from_f3d(mat['f3d_mat']['prim_color'])

    result.tex0 = _determine_tex_from_f3d(mat['f3d_mat']['tex0'])
    result.tex1 = _determine_tex_from_f3d(mat['f3d_mat']['tex1'])

    if rdp_settings['g_cull_back']:
        result.culling = True
    else:
        result.culling = False

    return result


def load_material_with_name(material_name: str, bpy_mat: bpy.types.Material) -> material.Material:
    if not bpy_mat:
        return material.Material()

    material_filename = f"assets/{material_name}.mat.json"

    if not material_name.startswith('materials/'):
        # embedded material
        material_object = material.Material()

        if bpy_mat.use_nodes:
            determine_material_from_nodes(bpy_mat, material_object)

        return material_object

    elif os.path.exists(material_filename):
        material_object = material.parse_material(material_filename)
        return material_object
    elif 'f3d_mat' in bpy_mat:
        return determine_material_from_f3d(bpy_mat)
    else:
        raise Exception(f"{material_filename} does not exist")