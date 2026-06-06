import bpy
import os.path
import sys
import math
import re

from . import material
from . import serialize
from . import blend_modes
from . import filename

def search_node_input(from_node: bpy.types.Node, input_name: str):
    for input in from_node.inputs:
        if input.name == input_name:
            return input
        
    return None


def search_node_linkage(from_node: bpy.types.Node, input_name: str):
    input = search_node_input(from_node, input_name)

    if not input:
        return None

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
    result = math.floor(value * 255 + 0.5)

    if result > 255:
        result = 255
    elif result < 0:
        result = 0

    return result

def color_array_to_color(array):
    return material.Color(
        color_float_to_int(array[0] ** 0.454545),
        color_float_to_int(array[1] ** 0.454545),
        color_float_to_int(array[2] ** 0.454545),
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
        result.tex0.set_filename(image_path)

        if color_node.interpolation == 'Nearest':
            result.tex0.min_filter = 'nearest'
            result.tex0.mag_filter = 'nearest'
        else:
            result.tex0.min_filter = 'linear'
            result.tex0.mag_filter = 'linear'

        if color_node.extension == 'REPEAT':
            result.tex0.s.clamp = False
            result.tex0.s.mirror = False
            result.tex0.t.clamp = False
            result.tex0.t.mirror = False
        elif color_node.extension == 'MIRROR':
            result.tex0.s.clamp = False
            result.tex0.s.mirror = True
            result.tex0.t.clamp = False
            result.tex0.t.mirror = True
        else:
            result.tex0.s.clamp = True
            result.tex0.s.mirror = False
            result.tex0.t.clamp = True
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
        result.other_modes = material.OtherModes(material.BlendModeCycle(material.BlendColor.IN, material.BlendAlpha._0, material.BlendColor.IN, material.BlendMix._1), None)
        result.other_modes.alpha_compare = material.AlphaCompare.THRESHOLD
        result.blend_color = material.Color(0, 0, 0, 128)
    elif mat.blend_method == 'BLEND':
        result.other_modes = material.OtherModes(material.BlendModeCycle(material.BlendColor.IN, material.BlendAlpha.IN_A, material.BlendColor.MEMORY, material.BlendMix.INV_MUX_A), None)
        result.other_modes.z_write = False
    elif mat.blend_method == 'HASHED':
        result.other_modes = material.OtherModes(material.BlendModeCycle(material.BlendColor.IN, material.BlendAlpha._0, material.BlendColor.IN, material.BlendMix._1), None)
        result.other_modes.alpha_compare = material.AlphaCompare.DITHER
    else:
        result.other_modes = material.OtherModes(material.BlendModeCycle(material.BlendColor.IN, material.BlendAlpha._0, material.BlendColor.IN, material.BlendMix._1), None)

    if 'decal' in mat and mat['decal']:
        result.other_modes.z_mode = material.ZMode.DECAL
        result.other_modes.z_write = False

combA = {
    "COMBINED": material.CombineA.COMBINED,
    "TEXEL0": material.CombineA.TEX0,
    "TEXEL1": material.CombineA.TEX1,
    "PRIMITIVE": material.CombineA.PRIM,
    "SHADE": material.CombineA.SHADE,
    "ENVIRONMENT": material.CombineA.ENV,
    "1": material.CombineA._1,
    "NOISE": material.CombineA.NOISE,
    "0": material.CombineA._0,
}

combB = {
    "COMBINED": material.CombineB.COMBINED,
    "TEXEL0": material.CombineB.TEX0,
    "TEXEL1": material.CombineB.TEX1,
    "PRIMITIVE": material.CombineB.PRIM,
    "SHADE": material.CombineB.SHADE,
    "ENVIRONMENT": material.CombineB.ENV,
    "CENTER": material.CombineB.KEYCENTER,
    "K4": material.CombineB.K4,
    "0": material.CombineB._0,
}

combC = {
    "COMBINED": material.CombineC.COMBINED,
    "TEXEL0": material.CombineC.TEX0,
    "TEXEL1": material.CombineC.TEX1,
    "PRIMITIVE": material.CombineC.PRIM,
    "SHADE": material.CombineC.SHADE,
    "ENVIRONMENT": material.CombineC.ENV,
    "SCALE": material.CombineC.KEYSCALE,
    "COMBINED_ALPHA": material.CombineC.COMBINED_ALPHA,
    "TEXEL0_ALPHA": material.CombineC.TEX0_ALPHA,
    "TEXEL1_ALPHA": material.CombineC.TEX1_ALPHA,
    "PRIMITIVE_ALPHA": material.CombineC.PRIM_ALPHA,
    "SHADE_ALPHA": material.CombineC.SHADE_ALPHA,
    "ENV_ALPHA": material.CombineC.ENV_ALPHA,
    "LOD_FRACTION": material.CombineC.LOD_FRAC,
    "PRIM_LOD_FRAC": material.CombineC.PRIM_LOD_FRAC,
    "K5": material.CombineC.K5,
    "0": material.CombineC._0,
}

combD = {
    "COMBINED": material.CombineD.COMBINED,
    "TEXEL0": material.CombineD.TEX0,
    "TEXEL1": material.CombineD.TEX1,
    "PRIMITIVE": material.CombineD.PRIM,
    "SHADE": material.CombineD.SHADE,
    "ENVIRONMENT": material.CombineD.ENV,
    "1": material.CombineD._1,
    "0": material.CombineD._0,
}

aComb = {
    "COMBINED": material.ACombine.COMBINED,
    "TEXEL0": material.ACombine.TEX0,
    "TEXEL1": material.ACombine.TEX1,
    "PRIMITIVE": material.ACombine.PRIM,
    "SHADE": material.ACombine.SHADE,
    "ENVIRONMENT": material.ACombine.ENV,
    "1": material.ACombine._1,
    "0": material.ACombine._0,
}

aCombMul = {
    "LOD_FRACTION": material.ACombineMul.LOD_FRAC,
    "TEXEL0": material.ACombineMul.TEX0,
    "TEXEL1": material.ACombineMul.TEX1,
    "PRIMITIVE": material.ACombineMul.PRIM,
    "SHADE": material.ACombineMul.SHADE,
    "ENVIRONMENT": material.ACombineMul.ENV,
    "PRIM_LOD_FRAC": material.ACombineMul.PRIM_LOD_FRAC,
    "0": material.ACombineMul._0,
}

enumBlendColor = {
    'G_BL_CLR_IN': material.BlendColor.IN,
    'G_BL_CLR_MEM': material.BlendColor.MEMORY,
    'G_BL_CLR_BL': material.BlendColor.BLEND,
    'G_BL_CLR_FOG': material.BlendColor.FOG,
}

enumBlendAlpha = {
    'G_BL_A_IN': material.BlendAlpha.IN_A,
    'G_BL_A_FOG': material.BlendAlpha.FOG_A,
    'G_BL_A_SHADE': material.BlendAlpha.SHADE_A,
    'G_BL_0': material.BlendAlpha._0,
}

enumBlendMix = {
    'G_BL_1MA': material.BlendMix.INV_MUX_A,
    'G_BL_A_MEM': material.BlendMix.MEM_A,
    'G_BL_1': material.BlendMix._1,
    'G_BL_0': material.BlendMix._0,
}

cycleType = {
    'G_CYC_1CYCLE': material.CycleType.CYCLE_1,
    'G_CYC_2CYCLE': material.CycleType.CYCLE_2,
    'G_CYC_COPY': material.CycleType.CYCLE_COPY,
    'G_CYC_FILL': material.CycleType.CYCLE_FILL,
}

def _lookup_mixed_enum(strMapping: dict, enum, value):
    if isinstance(value, str):
        return strMapping[value]
    if isinstance(value, int):
        return enum(value)
    
    raise Exception(f"unknown type {value}")

def _lookup_mix_bool(strTrue: str, value) -> bool:
    if isinstance(value, str):
        return value == strTrue
    if isinstance(value, int):
        return value == 1
    
    raise Exception(f"unknown type {value}")

def _determine_combiner_from_f3d(combiner) -> material.CombineModeCycle:
    return material.CombineModeCycle(
        _lookup_mixed_enum(combA, material.CombineA, combiner['A']),
        _lookup_mixed_enum(combB, material.CombineB, combiner['B']),
        _lookup_mixed_enum(combC, material.CombineC, combiner['C']),
        _lookup_mixed_enum(combD, material.CombineD, combiner['D']),
        _lookup_mixed_enum(aComb, material.ACombine, combiner['A_alpha']),
        _lookup_mixed_enum(aComb, material.ACombine, combiner['B_alpha']),
        _lookup_mixed_enum(aCombMul, material.ACombineMul, combiner['C_alpha']),
        _lookup_mixed_enum(aComb, material.ACombine, combiner['D_alpha']),
    )

def _determine_color_from_f3d(color) -> material.Color:
    return material.Color(
        int((color[0] ** 0.454545) * 255),
        int((color[1] ** 0.454545) * 255),
        int((color[2] ** 0.454545) * 255),
        int(color[3] * 255)
    )

def _determine_tex_axis_from_f3d(axis, image_size, uv_scroll, result: material.TexAxis):
    result.shift = axis['shift']

    result.mask = material.log_pow_2(image_size)
    if 'clamp' in axis and axis['clamp']:
        result.clamp = True
    
    if 'mirror' in axis and axis['mirror']:
        result.mirror = True

    if uv_scroll and uv_scroll['animType'] == 1:
        result.scroll = uv_scroll['speed'] if 'speed' in uv_scroll else 1

    result.min = 0
    result.max = image_size << 2

filename_number_suffix = r'\d+\.png$'

def _look_for_frames(filename: str) -> list[str]:
    match = re.search(filename_number_suffix, filename)

    if not match:
        return [filename]

    number_text = filename[match.start() - match.endpos: -4]
    prefix = filename[0:match.start() - match.endpos]

    pad_len = len(number_text)

    result = []
    index = int(number_text)

    while index < 100:
        frame_filename = prefix + str(index).rjust(pad_len, '0') + '.png'

        if not os.path.isfile(frame_filename):
            break

        result.append(frame_filename)
        index += 1

    return result

def _determine_tex_from_f3d(tex, uv_scroll, base_path: str, flipbook: bool = False) -> material.Tex | None:
    image = tex['tex']

    if not tex['tex_set'] or not image:
        return None

    result = material.Tex()

    filename = os.path.normpath(os.path.join(os.path.dirname(base_path), image.filepath[2:]))
    frames = _look_for_frames(filename) if flipbook else [filename]

    if len(frames) == 1:
        result.set_filename(filename)
    else:
        result.set_frames(frames)

    _determine_tex_axis_from_f3d(tex['S'], result.width, uv_scroll['x'] if uv_scroll and 'x' in uv_scroll else None, result.s)
    _determine_tex_axis_from_f3d(tex['T'], result.height, uv_scroll['y'] if uv_scroll and 'y' in uv_scroll else None, result.t)

    if 'tile_scroll' in tex:
        s_scroll = tex['tile_scroll'].get('s') or 0
        t_scroll = tex['tile_scroll'].get('t') or 0

        interval = tex['tile_scroll'].get('interval') or 1

        if s_scroll and interval:
            result.s.scroll += s_scroll / interval
        if t_scroll and interval:
            result.t.scroll += t_scroll / interval

    return result

_CYCLE_1_PRESETS = {
    0: blend_modes.RM_ZB_OPA_SURF,
    'G_RM_ZB_OPA_SURF': blend_modes.RM_ZB_OPA_SURF,
    1: blend_modes.RM_AA_ZB_OPA_SURF,
    'G_RM_AA_ZB_OPA_SURF': blend_modes.RM_AA_ZB_OPA_SURF,
    2: blend_modes.RM_AA_ZB_OPA_DECAL,
    'G_RM_AA_ZB_OPA_DECAL': blend_modes.RM_AA_ZB_OPA_DECAL,
    3: blend_modes.RM_AA_ZB_OPA_INTER,
    'G_RM_AA_ZB_OPA_INTER': blend_modes.RM_AA_ZB_OPA_INTER,
    4: blend_modes.RM_AA_ZB_TEX_EDGE,
    'G_RM_AA_ZB_TEX_EDGE': blend_modes.RM_AA_ZB_TEX_EDGE,
    5: blend_modes.RM_AA_ZB_XLU_SURF,
    'G_RM_AA_ZB_XLU_SURF': blend_modes.RM_AA_ZB_XLU_SURF,
    6: blend_modes.RM_AA_ZB_XLU_DECAL,
    'G_RM_AA_ZB_XLU_DECAL': blend_modes.RM_AA_ZB_XLU_DECAL,
    7: blend_modes.RM_AA_ZB_XLU_INTER,
    'G_RM_AA_ZB_XLU_INTER': blend_modes.RM_AA_ZB_XLU_INTER,
    8: blend_modes.RM_FOG_SHADE_A,
    'G_RM_FOG_SHADE_A': blend_modes.RM_FOG_SHADE_A,
    9: blend_modes.RM_FOG_PRIM_A,
    'G_RM_FOG_PRIM_A': blend_modes.RM_FOG_PRIM_A,
    10: blend_modes.RM_PASS,
    'G_RM_PASS': blend_modes.RM_PASS,
    11: blend_modes.RM_ADD,
    'G_RM_ADD': blend_modes.RM_ADD,
    12: blend_modes.RM_NOOP,
    'G_RM_NOOP': blend_modes.RM_NOOP,
    13: blend_modes.RM_ZB_OPA_SURF,
    'G_RM_ZB_OPA_SURF': blend_modes.RM_ZB_OPA_SURF,
    14: blend_modes.RM_ZB_OPA_DECAL,
    'G_RM_ZB_OPA_DECAL': blend_modes.RM_ZB_OPA_DECAL,
    15: blend_modes.RM_ZB_XLU_SURF,
    'G_RM_ZB_XLU_SURF': blend_modes.RM_ZB_XLU_SURF,
    16: blend_modes.RM_ZB_XLU_DECAL,
    'G_RM_ZB_XLU_DECAL': blend_modes.RM_ZB_XLU_DECAL,
    17: blend_modes.RM_OPA_SURF,
    'G_RM_OPA_SURF': blend_modes.RM_OPA_SURF,
    18: blend_modes.RM_ZB_CLD_SURF,
    'G_RM_ZB_CLD_SURF': blend_modes.RM_ZB_CLD_SURF,
    19: blend_modes.RM_AA_ZB_TEX_TERR,
    'G_RM_AA_ZB_TEX_TERR': blend_modes.RM_AA_ZB_TEX_TERR,
}

_CYCLE_2_PRESETS = {
    0: blend_modes.RM_ZB_OPA_SURF,
    'G_RM_ZB_OPA_SURF2': blend_modes.RM_ZB_OPA_SURF,
    1: blend_modes.RM_AA_ZB_OPA_SURF,
    'G_RM_AA_ZB_OPA_SURF2': blend_modes.RM_AA_ZB_OPA_SURF,
    2: blend_modes.RM_AA_ZB_OPA_DECAL,
    'G_RM_AA_ZB_OPA_DECAL2': blend_modes.RM_AA_ZB_OPA_DECAL,
    3: blend_modes.RM_AA_ZB_OPA_INTER,
    'G_RM_AA_ZB_OPA_INTER2': blend_modes.RM_AA_ZB_OPA_INTER,
    4: blend_modes.RM_AA_ZB_TEX_EDGE,
    'G_RM_AA_ZB_TEX_EDGE2': blend_modes.RM_AA_ZB_TEX_EDGE,
    5: blend_modes.RM_AA_ZB_XLU_SURF,
    'G_RM_AA_ZB_XLU_SURF2': blend_modes.RM_AA_ZB_XLU_SURF,
    6: blend_modes.RM_AA_ZB_XLU_DECAL,
    'G_RM_AA_ZB_XLU_DECAL2': blend_modes.RM_AA_ZB_XLU_DECAL,
    7: blend_modes.RM_AA_ZB_XLU_INTER,
    'G_RM_AA_ZB_XLU_INTER2': blend_modes.RM_AA_ZB_XLU_INTER,
    8: blend_modes.RM_ADD,
    'G_RM_ADD2': blend_modes.RM_ADD,
    9: blend_modes.RM_NOOP,
    'G_RM_NOOP2': blend_modes.RM_NOOP,
    10: blend_modes.RM_ZB_OPA_SURF,
    'G_RM_ZB_OPA_SURF2': blend_modes.RM_ZB_OPA_SURF,
    11: blend_modes.RM_ZB_OPA_DECAL,
    'G_RM_ZB_OPA_DECAL2': blend_modes.RM_ZB_OPA_DECAL,
    12: blend_modes.RM_ZB_XLU_SURF,
    'G_RM_ZB_XLU_SURF2': blend_modes.RM_ZB_XLU_SURF,
    13: blend_modes.RM_ZB_XLU_DECAL,
    'G_RM_ZB_XLU_DECAL2': blend_modes.RM_ZB_XLU_DECAL,
    14: blend_modes.RM_ZB_CLD_SURF,
    'G_RM_ZB_CLD_SURF2': blend_modes.RM_ZB_CLD_SURF,
    15: blend_modes.RM_ZB_OVL_SURF,
    'G_RM_ZB_OVL_SURF2': blend_modes.RM_ZB_OVL_SURF,
    16: blend_modes.RM_AA_ZB_TEX_TERR,
    'G_RM_AA_ZB_TEX_TERR2': blend_modes.RM_AA_ZB_TEX_TERR,
    17: blend_modes.RM_OPA_SURF,
    'G_RM_OPA_SURF2': blend_modes.RM_OPA_SURF,
}

texDetail = {
    'G_TD_CLAMP': material.TextureDetail.CLAMP,
    'G_TD_SHARPEN': material.TextureDetail.SHARPEN,
    'G_TD_DETAIL': material.TextureDetail.DETAIL,
}

zMode = {
    'ZMODE_OPA': material.ZMode.OPAQUE,
    'ZMODE_INTER': material.ZMode.INTER,
    'ZMODE_XLU': material.ZMode.TRANSPARENT,
    'ZMODE_DEC': material.ZMode.DECAL,
}

cvgDest = {
    'CVG_DST_CLAMP': material.CvgDest.CLAMP,
    'CVG_DST_WRAP': material.CvgDest.WRAP,
    'CVG_DST_FULL': material.CvgDest.FULL,
    'CVG_DST_SAVE': material.CvgDest.SAVE,
}

sampleType = {
    'G_TF_POINT': material.SampleType.POINT,
    'G_TF_AVERAGE': material.SampleType.MEDIAN,
    'G_TF_BILERP': material.SampleType.BILINEAR,
}

rgbDither = {
    'G_CD_MAGICSQ': material.RgbDitherSel.MAGIC_SQUARE,
    'G_CD_BAYER': material.RgbDitherSel.STANDARD,
    'G_CD_NOISE': material.RgbDitherSel.NOISE,
    'G_CD_DISABLE': material.RgbDitherSel.NONE,
    'G_CD_ENABLE': material.RgbDitherSel.MAGIC_SQUARE,
}

alphaDither = {
    'G_AD_PATTERN': material.AlphaDitherSel.PATTERN,
    'G_AD_NOTPATTERN': material.AlphaDitherSel.INV_PATTERN,
    'G_AD_NOISE': material.AlphaDitherSel.NOISE,
    'G_AD_DISABLE': material.AlphaDitherSel.NONE,
}

zSourceSel = {
    'G_ZS_PIXEL': material.ZSourceSel.PIXEL,
    'G_ZS_PRIM': material.ZSourceSel.PRIM,
}

alphaCompare = {
    'G_AC_NONE': material.AlphaCompare.NONE,
    'G_AC_THRESHOLD': material.AlphaCompare.THRESHOLD,
    'G_AC_DITHER': material.AlphaCompare.DITHER,
}

def determine_material_blend_f3d(rdp_settings) -> material.OtherModes:
    is_2_cycle = _lookup_mix_bool('G_CYC_2CYCLE', rdp_settings['g_mdsft_cycletype'])

    if rdp_settings['rendermode_advanced_enabled']:
        return material.OtherModes(
            material.BlendModeCycle(
                _lookup_mixed_enum(enumBlendColor, material.BlendColor, rdp_settings['blend_p1']), 
                _lookup_mixed_enum(enumBlendAlpha, material.BlendAlpha, rdp_settings['blend_a1']), 
                _lookup_mixed_enum(enumBlendColor, material.BlendColor, rdp_settings['blend_m1']), 
                _lookup_mixed_enum(enumBlendMix, material.BlendMix, rdp_settings['blend_b1']),
            ),
            material.BlendModeCycle(
                _lookup_mixed_enum(enumBlendColor, material.BlendColor, rdp_settings['blend_p2']), 
                _lookup_mixed_enum(enumBlendAlpha, material.BlendAlpha, rdp_settings['blend_a2']), 
                _lookup_mixed_enum(enumBlendColor, material.BlendColor, rdp_settings['blend_m2']), 
                _lookup_mixed_enum(enumBlendMix, material.BlendMix, rdp_settings['blend_b2'])
            ) if is_2_cycle else None,
            atomic_prim = _lookup_mix_bool('G_PM_NPRIMITIVE', rdp_settings['g_mdsft_pipeline']),
            cycle_type = _lookup_mixed_enum(cycleType, material.CycleType, rdp_settings['g_mdsft_cycletype']),
            persp_tex_en = _lookup_mix_bool('G_TP_PERSP', rdp_settings['g_mdsft_textpersp']),
            tex_detail = _lookup_mixed_enum(texDetail, material.TextureDetail, rdp_settings['g_mdsft_textdetail']),
            tex_lod_en = _lookup_mix_bool('G_TL_LOD', rdp_settings['g_mdsft_textlod']),
            sample_type = _lookup_mixed_enum(sampleType, material.SampleType, rdp_settings['g_mdsft_text_filt']),
            key_en = _lookup_mix_bool('G_CK_KEY', rdp_settings['g_mdsft_combkey']),
            rgb_dither_sel = _lookup_mixed_enum(rgbDither, material.RgbDitherSel, rdp_settings['g_mdsft_rgb_dither']),
            alpha_dither_sel = _lookup_mixed_enum(alphaDither, material.AlphaDitherSel, rdp_settings['g_mdsft_alpha_dither']),
            force_blend = bool(rdp_settings['force_bl']),
            alpha_coverage = bool(rdp_settings['alpha_cvg_sel']),
            x_coverage_alpha = bool(rdp_settings['cvg_x_alpha']),
            z_mode = _lookup_mixed_enum(zMode, material.ZMode, rdp_settings['zmode']),
            coverage_dest = _lookup_mixed_enum(cvgDest, material.CvgDest, rdp_settings['cvg_dst']),
            color_on_coverage = bool(rdp_settings['clr_on_cvg']),
            image_read = bool(rdp_settings['im_rd']),
            z_write = bool(rdp_settings['z_upd']),
            z_compare = bool(rdp_settings['z_cmp']),
            aa = bool(rdp_settings['aa_en']),
            z_source_sel = _lookup_mixed_enum(zSourceSel, material.ZSourceSel, rdp_settings['g_mdsft_zsrcsel']),
            alpha_compare = _lookup_mixed_enum(alphaCompare, material.AlphaCompare, rdp_settings['g_mdsft_alpha_compare']),

            # TODO
            # yuv_en = False,
        )
    else:
        result = blend_modes.combine_blend_mode(
            _CYCLE_1_PRESETS[rdp_settings['rendermode_preset_cycle_1']],
            _CYCLE_2_PRESETS[rdp_settings['rendermode_preset_cycle_2']] if is_2_cycle else None
        )
        result.alpha_compare = _lookup_mixed_enum(alphaCompare, material.AlphaCompare, rdp_settings['g_mdsft_alpha_compare'])
        result.persp_tex_en = _lookup_mix_bool('g_mdsft_textpersp', rdp_settings['g_mdsft_textpersp'])
        result.sample_type = _lookup_mixed_enum(sampleType, material.SampleType, rdp_settings['g_mdsft_text_filt'])
        result.rgb_dither_sel = _lookup_mixed_enum(rgbDither, material.RgbDitherSel, rdp_settings['g_mdsft_rgb_dither'])
        result.alpha_dither_sel = _lookup_mixed_enum(alphaDither, material.AlphaDitherSel, rdp_settings['g_mdsft_alpha_dither'])
        result.z_source_sel = _lookup_mixed_enum(zSourceSel, material.ZSourceSel, rdp_settings['g_mdsft_zsrcsel'])
        result.tex_detail = _lookup_mixed_enum(texDetail, material.TextureDetail, rdp_settings['g_mdsft_textdetail'])
        result.tex_lod_en = _lookup_mix_bool('G_TL_LOD', rdp_settings['g_mdsft_textlod'])
        result.atomic_prim = _lookup_mix_bool('G_PM_NPRIMITIVE', rdp_settings['g_mdsft_pipeline'])
        result.cycle_type = _lookup_mixed_enum(cycleType, material.CycleType, rdp_settings['g_mdsft_cycletype'])
        result.persp_tex_en = _lookup_mix_bool('G_TP_PERSP', rdp_settings['g_mdsft_textpersp'])
        result.tex_detail = _lookup_mixed_enum(texDetail, material.TextureDetail, rdp_settings['g_mdsft_textdetail'])
        result.tex_lod_en = _lookup_mix_bool('G_TL_LOD', rdp_settings['g_mdsft_textlod'])
        result.key_en = _lookup_mix_bool('G_CK_KEY', rdp_settings['g_mdsft_combkey'])
        return result
    
class PropertyWalker():
    def __init__(self, obj):
        self.obj = obj

    def __getitem__(self, key: str):
        result = None

        if isinstance(key, str) and hasattr(self.obj, key):
            result = getattr(self.obj, key)
        elif key in self.obj:
            result = self[key]

        if result == None:
            return None
        
        type_name = type(result).__name__
        
        if isinstance(result, int) or isinstance(result, str) or isinstance(result, float) or isinstance(result, bool) or type_name == 'bpy_prop_array' or type_name == 'Image':
            return result
        
        return PropertyWalker(result)
    
    def __contains__(self, item):
        return item in self.obj or isinstance(item, str) and hasattr(self.obj, item)
        

def determine_material_from_f3d(mat: bpy.types.Material) -> material.Material:
    f3d_mat = PropertyWalker(mat)['f3d_mat']

    rdp_settings = f3d_mat['rdp_settings']

    is_2_cycle = _lookup_mix_bool('G_CYC_2CYCLE', rdp_settings['g_mdsft_cycletype'])

    base_path = sys.argv[1]

    if mat.library:
        base_path = os.path.normpath(os.path.join(os.path.dirname(base_path), mat.library.filepath[2:]))
    
    result: material.Material = material.Material()
    result.combine_mode = material.CombineMode(
        _determine_combiner_from_f3d(f3d_mat['combiner1']),
        _determine_combiner_from_f3d(f3d_mat['combiner2']) if is_2_cycle else None,
    )

    result.other_modes = determine_material_blend_f3d(rdp_settings)

    if f3d_mat['set_env']:
        result.env_color = _determine_color_from_f3d(f3d_mat['env_color'])

    if f3d_mat['set_prim']:
        result.prim_color = _determine_color_from_f3d(f3d_mat['prim_color'])

    if f3d_mat['set_blend']:
        result.blend_color = _determine_color_from_f3d(f3d_mat['blend_color'])

    uv_anim_0 = f3d_mat['UVanim0'] if 'UVanim0' in f3d_mat else None
    uv_anim_1 = f3d_mat['UVanim1'] if 'UVanim1' in f3d_mat else None

    flipbook_0 = 'flipbook_0' in mat and mat['flipbook_0']

    result.tex0 = _determine_tex_from_f3d(f3d_mat['tex0'], uv_anim_0, base_path, flipbook = flipbook_0) if result.combine_mode.uses('TEX0') else None
    result.tex1 = _determine_tex_from_f3d(f3d_mat['tex1'], uv_anim_1, base_path) if result.combine_mode.uses('TEX1') else None
    result.layout_textures()

    if result.tex0 and result.tex0.palette_data or result.tex1 and result.tex1.palette_data:
        result.other_modes.en_tlut = True
        result.other_modes.tlut_type = material.TlutType.TLUT_RGBA

    if rdp_settings['g_cull_back']:
        result.culling = True
    else:
        result.culling = False

    if rdp_settings['g_tex_gen']:
        use_tex = result.tex0 or result.tex1

        if not use_tex:
            raise Exception('Cannot do spherical uv without a texture')

        result.vtx_effect = material.VtxEffect(material.VtxEffectType.VTX_EFFECT_SPHERICAL, use_tex.width, use_tex.height)
    else:
        result.vtx_effect = material.VtxEffect(material.VtxEffectType.VTX_EFFECT_NONE)

    result.fog = material.Fog()
    result.fog.enabled = rdp_settings['g_fog'] and True or False
    result.fog.use_global = f3d_mat['use_global_fog'] and True or False
    result.fog.fog_color = _determine_color_from_f3d(f3d_mat['fog_color'])
    result.fog.min_distance = f3d_mat['fog_position'][0]
    result.fog.max_distance = f3d_mat['fog_position'][1]

    if f3d_mat['set_lights']:
        result.light_count = 1
    else:
        result.light_count = 0

    if 'priority' in mat:
        result.priority = mat['priority']
    else:
        result.priority = int(f3d_mat['draw_layer']['sm64'])

    return result

def material_can_extract(bpy_mat: bpy.types.Material) -> bool:
    return bpy_mat.name.startswith('materials/') or 'f3d_mat' in bpy_mat

def material_romname(bpy_mat: bpy.types.Material) -> str | None:
    if bpy_mat.name.startswith('materials/'):
        material_filename = f"assets/{bpy_mat.name}.mat.json"

        if os.path.exists(material_filename):
            return f"rom:/{bpy_mat.name}.mat"
        else:
            raise Exception(f"{material_filename} does not exist")
        
    if bpy_mat.library:
        return filename.rom_filename(bpy_mat.library.filepath)
    
    return None
        

def load_material_with_name(bpy_mat: bpy.types.Material) -> material.Material:
    if not bpy_mat:
        return material.Material()
    
    material_name = bpy_mat.name

    material_filename = f"assets/{material_name}.mat.json"

    if os.path.exists(material_filename):
        material_object = material.parse_material(material_filename)
        return material_object
    elif 'f3d_mat' in bpy_mat:
        return determine_material_from_f3d(bpy_mat)
    elif not material_name.startswith('materials/'):
        # embedded material
        material_object = material.Material()

        if bpy_mat.use_nodes:
            determine_material_from_nodes(bpy_mat, material_object)

        return material_object
    else:
        raise Exception(f"{material_filename} does not exist")