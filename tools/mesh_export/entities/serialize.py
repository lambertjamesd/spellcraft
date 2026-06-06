import sys
import os

from . import material
import struct

COMMAND_EOF = 0
COMMAND_COMBINE = 1
COMMAND_BLEND = 2
COMMAND_ENV = 3
COMMAND_PRIM = 4
COMMAND_BLEND_COLOR = 5
COMMAND_FLAGS = 6
COMMAND_PALETTE = 7
COMMAND_UV_GEN = 8
COMMAND_FOG = 9
COMMAND_FOG_COLOR = 10
COMMAND_FOG_RANGE = 11
COMMAND_LIGHT_COUNT = 12

T3D_FLAG_DEPTH      = 1 << 0
T3D_FLAG_TEXTURED   = 1 << 1
T3D_FLAG_SHADED     = 1 << 2
T3D_FLAG_CULL_FRONT = 1 << 3
T3D_FLAG_CULL_BACK  = 1 << 4

def _serialize_color(file, color: material.Color):
    file.write(struct.pack('>BBBB', color.r, color.g, color.b, color.a))

ZMODE = {
    "OPAQUE": 0 << 10,
    "INTER": 1 << 10,
    "TRANSPARENT": 2 << 10,
    "DECAL": 3 << 10,
}

COVERAGE_DEST = {
    'CLAMP': 0 << 8,
    'WRAP': 1 << 8,
    'ZAP': 2 << 8,
    'FULL': 2 << 8,
    'SAVE': 3 << 8,
}

ALPHACOMPARE = {
    "NONE": 0,
    "THRESHOLD": 1,
    "NOISE": 3,
    "DITHER": 3,
}

CYCLE_TYPE = {
    "CYCLE_1": 0 << 52,
    "CYCLE_2": 1 << 52,
    "CYCLE_COPY": 2 << 52,
    "CYCLE_FILL": 3 << 52,
}

TLUT_TYPE = {
    "TLUT_RGBA": 0 << 46,
    "TLUTIA": 1 << 46,
}

SAMPLE_TYPE = {
    "POINT": 0 << 44,
    "MEDIAN": 3 << 44,
    "BILINEAR": 2 << 44,
}

TEX_FMT = {
    'FMT_NONE': 0,
    'FMT_RGBA16': 2,
    'FMT_RGBA32': 3,
    'FMT_YUV16': 6,
    'FMT_CI4': 8,
    'FMT_CI8': 9,
    'FMT_IA4': 12,
    'FMT_IA8': 13,
    'FMT_IA16': 14,
    'FMT_I4': 16,
    'FMT_I8': 17,
}

RGB_DITHER = {
    "MAGIC_SQUARE": 0 << 38,
    "STANDARD": 1 << 38,
    "NOISE": 2 << 38,
    "NONE": 3 << 38,
}

ALPHA_DITHER = {
    "PATTERN": 0 << 36,
    "INV_PATTERN": 1 << 36,
    "NOISE": 2 << 36,
    "NONE": 3 << 36,
}

Z_SOURCE = {
    "PIXEL": 0 << 2,
    "PRIM": 1 << 2,
}

TEX_DETAIL = {
    "CLAMP": 0 << 49,
    "SHARPEN": 1 << 49,
    "DETAIL": 2 << 49,
}

RDPQ_COMBINER_2PASS = 1 << 63

def _serialize_combine(file, combine: material.CombineMode, force_cyc2: bool):
    if not combine.cyc1:
        raise Exception('needs cycle 1 to serialize combine')

    a0 = combine.cyc1.a.value
    b0 = combine.cyc1.b.value
    c0 = combine.cyc1.c.value
    d0 = combine.cyc1.d.value

    aa0 = combine.cyc1.aa.value
    ab0 = combine.cyc1.ab.value
    ac0 = combine.cyc1.ac.value
    ad0 = combine.cyc1.ad.value

    a1 = a0
    b1 = b0
    c1 = c0
    d1 = d0

    aa1 = aa0
    ab1 = ab0
    ac1 = ac0
    ad1 = ad0

    flags = 0

    if combine.cyc2:
        a1 = combine.cyc2.a.value
        b1 = combine.cyc2.b.value
        c1 = combine.cyc2.c.value
        d1 = combine.cyc2.d.value

        aa1 = combine.cyc2.aa.value
        ab1 = combine.cyc2.ab.value
        ac1 = combine.cyc2.ac.value
        ad1 = combine.cyc2.ad.value
    elif force_cyc2:
        a1 = material.CombineA._0.value
        b1 = material.CombineB._0.value
        c1 = material.CombineC._0.value
        d1 = material.CombineC.COMBINED.value

        aa1 = material.ACombine._0.value
        ab1 = material.ACombine._0.value
        ac1 = material.ACombineMul._0.value
        ad1 = material.ACombine.COMBINED.value


    file.write(struct.pack('>Q', \
        (a0 << 52) | (b0 << 28) | (c0 << 47) | (d0 << 15) | \
        (aa0 << 44) | (ab0 << 12) | (ac0 << 41) | (ad0 << 9) | \
        (a1 << 37) | (b1 << 24) | (c1 << 32) | (d1 << 6) |     \
        (aa1 << 21) | (ab1 << 3) | (ac1 << 18) | (ad1 << 0) | \
        flags \
    ))

SOM_ATOMIC_PRIM = 1 << 55
SOM_TEXTURE_PERSP = 1 << 51
SOM_TEXTURE_LOD = 1 << 48
SOM_TLUT = 1 << 47
SOM_TF0 = 1 << 43
SOM_TF1 = 1 << 42
SOM_TF1YUV = 1 << 41
SOM_CHROMA_KEY = 1 << 40

SOM_BLENDING = 1 << 14
SOM_ALPHA_COVERAGE = 1 << 13
SOM_CVG_TIMES_ALPHA = 1 << 12
SOM_COLOR_ON_CVG_OVERFLOW = 1 << 7
SOM_READ_ENABLE = 1 << 6
SOM_Z_WRITE = 1 << 5
SOM_Z_COMPARE = 1 << 4
SOM_AA = 1 << 3

def _serialize_other_modes(file, blend: material.OtherModes, force_cyc2: bool):
    a1 = blend.cyc1.a1.value
    b1 = blend.cyc1.b1.value
    a2 = blend.cyc1.a2.value
    b2 = blend.cyc1.b2.value

    a1_2 = a1
    b1_2 = b1
    a2_2 = a2
    b2_2 = b2

    other_flags = ZMODE[blend.z_mode.name] | \
        ALPHACOMPARE[blend.alpha_compare.name] | \
        TLUT_TYPE[blend.tlut_type.name] | \
        SAMPLE_TYPE[blend.sample_type.name] | \
        RGB_DITHER[blend.rgb_dither_sel.name] | \
        ALPHA_DITHER[blend.alpha_dither_sel.name] | \
        COVERAGE_DEST[blend.coverage_dest.name] | \
        Z_SOURCE[blend.z_source_sel.name] | \
        TEX_DETAIL[blend.tex_detail.name]

    if blend.atomic_prim:
        other_flags |= SOM_ATOMIC_PRIM
        
    if blend.persp_tex_en:
        other_flags |= SOM_TEXTURE_PERSP
        
    if blend.tex_lod_en:
        other_flags |= SOM_TEXTURE_LOD

    if blend.en_tlut:
        other_flags |= SOM_TLUT
        
    if blend.yuv_en:
        other_flags |= SOM_TF1YUV
    else:
        other_flags |= SOM_TF0 | SOM_TF1

    if blend.key_en:
        other_flags |= SOM_CHROMA_KEY

    if blend.force_blend or blend.should_blend():
        other_flags |= SOM_BLENDING

    if blend.alpha_coverage:
        other_flags |= SOM_ALPHA_COVERAGE
    
    if blend.x_coverage_alpha:
        other_flags |= SOM_CVG_TIMES_ALPHA

    if blend.color_on_coverage:
        other_flags |= SOM_COLOR_ON_CVG_OVERFLOW

    if blend.image_read or blend.cyc1.needs_read():
        other_flags |= SOM_READ_ENABLE

    if blend.z_write:
        other_flags |= SOM_Z_WRITE

    if blend.z_compare:
        other_flags |= SOM_Z_COMPARE

    if blend.aa:
        other_flags |= SOM_AA

    cycle_type = blend.cycle_type

    if blend.cyc2:
        a1_2 = blend.cyc2.a1.value
        b1_2 = blend.cyc2.b1.value
        a2_2 = blend.cyc2.a2.value
        b2_2 = blend.cyc2.b2.value

        if blend.cyc2.needs_read():
            other_flags |= SOM_READ_ENABLE
    elif force_cyc2:
        a1 = material.BlendColor.IN.value
        b1 = material.BlendAlpha._0.value
        a2 = material.BlendColor.IN.value
        b2 = material.BlendMix._1.value

        if cycle_type == material.CycleType.CYCLE_1:
            cycle_type = material.CycleType.CYCLE_2

    other_flags |= CYCLE_TYPE[cycle_type.name]

    file.write(struct.pack('>Q',
        (a1 << 30) | (b1 << 26) | (a2 << 22) | (b2 << 18) |
        (a1_2 << 28) | (b1_2 << 24) | (a2_2 << 20) | (b2_2 << 16) |
        other_flags    
    ))


def _serialze_string(file, text):
    encoded_text = text.encode()
    file.write(len(encoded_text).to_bytes(1, 'big'))
    file.write(encoded_text)

def _serialize_tex_axis(file, axis: material.TexAxis):
    file.write(b'\x01' if axis.clamp else b'\x00')
    file.write(b'\x01' if axis.mirror else b'\x00')
    file.write(int(axis.mask & 0xFF).to_bytes(1, 'big'))
    file.write(int(axis.shift & 0xFF).to_bytes(1, 'big'))

def _serialize_tex(file, tex: material.Tex | None, prev_tex: material.Tex | None = None):
    if not tex:
        file.write(b'\0')
        return
    
    file.write(b'\x01')

    if tex.filename and len(tex.filename):
        _serialze_string(file, tex.rom_filename())
    else:
        file.write(b'\0')
        
    file.write(tex.palette.to_bytes(1, 'big'))
    _serialize_tex_axis(file, tex.s)
    _serialize_tex_axis(file, tex.t)

    file.write(tex.tmem_addr.to_bytes(2, 'big'))
    file.write(TEX_FMT[tex.fmt].to_bytes(2, 'big'))

    file.write(tex.s.min.to_bytes(2, 'big'))
    file.write(tex.t.min.to_bytes(2, 'big'))
    file.write(tex.s.max.to_bytes(2, 'big'))
    file.write(tex.t.max.to_bytes(2, 'big'))

    file.write(tex.width.to_bytes(2, 'big'))
    file.write(tex.height.to_bytes(2, 'big'))

    if tex.frames:
        file.write(len(tex.frames).to_bytes(2, 'big'))
        for frame in tex.frame_filenames():
            _serialze_string(file, frame)
    else:
        file.write(b'\0\0')

    file.write(struct.pack('>ff', tex.s.scroll, -tex.t.scroll))

def _serialize_palette(file, palette: list, palette_offset: int):
    file.write(COMMAND_PALETTE.to_bytes(1, 'big'))
    file.write(palette_offset.to_bytes(2, 'big'))
    file.write(len(palette).to_bytes(2, 'big'))
    for color in palette:
        file.write(color.to_bytes(2, 'big'))

def flags_for_material(mat: material.Material) -> int:
    flags = 0

    if mat.other_modes and (mat.other_modes.z_compare or mat.other_modes.z_write):
        flags |= T3D_FLAG_DEPTH

    if mat.culling == 'front':
        flags |= T3D_FLAG_CULL_FRONT
    elif mat.culling == True:
        flags |= T3D_FLAG_CULL_BACK

    if mat.tex0 or mat.tex1:
        flags |= T3D_FLAG_TEXTURED

    if mat.combine_mode and mat.combine_mode.uses('SHADE'):
        flags |= T3D_FLAG_SHADED

    if mat.fog and mat.fog.enabled:
        flags |= T3D_FLAG_SHADED

    return flags

def serialize_material_file(output, mat: material.Material, current_state: material.Material | None = None):
    output.write('MATR'.encode())
    output.write(struct.pack('>h', mat.priority or 1))

    _serialize_tex(output, mat.tex0)
    _serialize_tex(output, mat.tex1, mat.tex0)

    force_cyc2 = False

    if mat.combine_mode and mat.combine_mode.cyc2:
        force_cyc2 = True

    if mat.other_modes and mat.other_modes.cyc2:
        force_cyc2 = True

    if mat.fog:
        force_cyc2 = True
    
    if mat.combine_mode:
        output.write(COMMAND_COMBINE.to_bytes(1, 'big'))
        _serialize_combine(output, mat.combine_mode, force_cyc2)

    if mat.other_modes:
        output.write(COMMAND_BLEND.to_bytes(1, 'big'))
        _serialize_other_modes(output, mat.other_modes, force_cyc2)
    
    if mat.env_color:
        output.write(COMMAND_ENV.to_bytes(1, 'big'))
        _serialize_color(output, mat.env_color)

    if mat.prim_color:
        output.write(COMMAND_PRIM.to_bytes(1, 'big'))
        _serialize_color(output, mat.prim_color)

    if mat.blend_color:
        output.write(COMMAND_BLEND_COLOR.to_bytes(1, 'big'))
        _serialize_color(output, mat.blend_color)

    palette_data, palette_offset = mat.get_palette_data()

    if palette_data:
        _serialize_palette(output, palette_data, palette_offset)

    if mat.vtx_effect:
        output.write(COMMAND_UV_GEN.to_bytes(1, 'big'))
        mat.vtx_effect.serialize(output)

    if mat.fog:
        output.write(COMMAND_FOG.to_bytes(1, 'big'))
        output.write((mat.fog.enabled and 1 or 0).to_bytes(1, 'big'))

        if not mat.fog.use_global:
            if mat.fog.fog_color:
                output.write(COMMAND_FOG_COLOR.to_bytes(1, 'big'))
                _serialize_color(output, mat.fog.fog_color)

            if mat.fog.min_distance != mat.fog.max_distance:
                output.write(COMMAND_FOG_RANGE.to_bytes(1, 'big'))
                output.write(int(mat.fog.min_distance).to_bytes(2, 'big'))
                output.write(int(mat.fog.max_distance).to_bytes(2, 'big'))

    flags = flags_for_material(mat)

    if current_state:
        flags |= flags_for_material(current_state)

    output.write(COMMAND_FLAGS.to_bytes(1, 'big'))
    output.write(flags.to_bytes(2, 'big'))

    if mat.light_count != None:
        output.write(COMMAND_LIGHT_COUNT.to_bytes(1, 'big'))
        output.write(mat.light_count.to_bytes(1, 'big'))

    output.write(COMMAND_EOF.to_bytes(1, 'big'))
