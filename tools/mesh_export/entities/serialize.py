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

T3D_FLAG_DEPTH      = 1 << 0
T3D_FLAG_TEXTURED   = 1 << 1
T3D_FLAG_SHADED     = 1 << 2
T3D_FLAG_CULL_FRONT = 1 << 3
T3D_FLAG_CULL_BACK  = 1 << 4

def _serialize_color(file, color: material.Color):
    file.write(struct.pack('>BBBB', color.r, color.g, color.b, color.a))

COMB_A = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "ONE": 6,
    "1": 6,
    "NOISE": 7,
    "ZERO": 8,
    "0": 8,
}

COMB_B = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "KEYCENTER": 6,
    "K4": 7,
    "ZERO": 8,
    "0": 8,
}

COMB_C = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "KEYSCALE": 6,
    "COMBINED_ALPHA": 7,
    "TEX0_ALPHA": 8,
    "TEX1_ALPHA": 9,
    "PRIM_ALPHA": 10,
    "SHADE_ALPHA": 11,
    "ENV_ALPHA": 12,
    "LOD_FRAC": 13,
    "PRIM_LOD_FRAC": 14,
    "K5": 15,
    "ZERO": 16,
    "0": 16,
}

COMB_D = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    
    "ONE": 6,
    "1": 6,
    "ZERO": 7,
    "0": 7,
}

ACOMB_A = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    
    "ONE": 6,
    "1": 6,
    "ZERO": 7,
    "0": 7,
}

ACOMB_MUL = {
    "LOD_FRAC": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "PRIM_LOD_FRAC": 6,
    "ZERO": 7,
    "0": 7,
}

BLEND_A = {
    "IN": 0,
    "MEMORY": 1,
    "BLEND": 2,
    "FOG": 3,
}

BLEND_B1 = {
    "IN_A": 0,
    "FOG_A": 1,
    "SHADE_A": 2,
    "ZERO": 3,
    "0": 3,
}

BLEND_B2 = {
    "INV_MUX_A": 0,
    "MEMORY_CVG": 1,
    "ONE": 2,
    "1": 2,
    "ZERO": 3,
    "0": 3,
}

ZMODE = {
    "OPAQUE": 0 << 10,
    "INTER": 1 << 10,
    "TRANSPARENT": 2 << 10,
    "DECAL": 3 << 10,
}

ALPHACOMPARE = {
    "NONE": 0,
    "THRESHOLD": 1,
    "NOISE": 3,
}

UV_GEN = {
    'none': 0,
    'spherical': 1,
}

RDPQ_COMBINER_2PASS = 1 << 63

def _serialize_combine(file, combine: material.CombineMode, force_cyc2: bool):
    a0 = COMB_A[combine.cyc1.a]
    b0 = COMB_B[combine.cyc1.b]
    c0 = COMB_C[combine.cyc1.c]
    d0 = COMB_D[combine.cyc1.d]

    aa0 = ACOMB_A[combine.cyc1.aa]
    ab0 = ACOMB_A[combine.cyc1.ab]
    ac0 = ACOMB_MUL[combine.cyc1.ac]
    ad0 = ACOMB_A[combine.cyc1.ad]

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
        a1 = COMB_A[combine.cyc2.a]
        b1 = COMB_B[combine.cyc2.b]
        c1 = COMB_C[combine.cyc2.c]
        d1 = COMB_D[combine.cyc2.d]

        aa1 = ACOMB_A[combine.cyc2.aa]
        ab1 = ACOMB_A[combine.cyc2.ab]
        ac1 = ACOMB_MUL[combine.cyc2.ac]
        ad1 = ACOMB_A[combine.cyc2.ad]

        flags |= RDPQ_COMBINER_2PASS
    elif force_cyc2:
        a1 = COMB_A['0']
        b1 = COMB_B['0']
        c1 = COMB_C['0']
        d1 = COMB_D['COMBINED']

        aa1 = ACOMB_A['0']
        ab1 = ACOMB_A['0']
        ac1 = ACOMB_MUL['0']
        ad1 = ACOMB_A['COMBINED']

        flags |= RDPQ_COMBINER_2PASS


    file.write(struct.pack('>Q', \
        (a0 << 52) | (b0 << 28) | (c0 << 47) | (d0 << 15) | \
        (aa0 << 44) | (ab0 << 12) | (ac0 << 41) | (ad0 << 9) | \
        (a1 << 37) | (b1 << 24) | (c1 << 32) | (d1 << 6) |     \
        (aa1 << 21) | (ab1 << 3) | (ac1 << 18) | (ad1 << 0) | \
        flags \
    ))

SOM_Z_COMPARE = 1 << 4
SOM_Z_WRITE = 1 << 5
SOM_READ_ENABLE = 1 << 6

def _serialize_blend(file, blend: material.BlendMode, force_cyc2: bool):
    a1 = BLEND_A[blend.cyc1.a1]
    b1 = BLEND_B1[blend.cyc1.b1]
    a2 = BLEND_A[blend.cyc1.a2]
    b2 = BLEND_B2[blend.cyc1.b2]

    a1_2 = a1
    b1_2 = b1
    a2_2 = a2
    b2_2 = b2

    other_flags = ZMODE[blend.z_mode] | ALPHACOMPARE[blend.alpha_compare]

    if blend.cyc1.needs_read():
        other_flags |= SOM_READ_ENABLE

    if blend.z_compare:
        other_flags |= SOM_Z_COMPARE

    if blend.z_write:
        other_flags |= SOM_Z_WRITE

    if blend.cyc2:
        a1_2 = BLEND_A[blend.cyc2.a1]
        b1_2 = BLEND_B1[blend.cyc2.b1]
        a2_2 = BLEND_A[blend.cyc2.a2]
        b2_2 = BLEND_B2[blend.cyc2.b2]

        if blend.cyc2.needs_read():
            other_flags |= SOM_READ_ENABLE
    elif force_cyc2:
        a1_2 = BLEND_A['IN']
        b1_2 = BLEND_B1['0']
        a2_2 = BLEND_A['IN']
        b2_2 = BLEND_B2['1']

    file.write(struct.pack('>L',
        (a1 << 30) | (b1 << 26) | (a2 << 22) | (b2 << 18) |
        (a1_2 << 28) | (b1_2 << 24) | (a2_2 << 20) | (b2_2 << 16) |
        other_flags    
    ))


def _serialze_string(file, text):
    encoded_text = text.encode()
    file.write(len(encoded_text).to_bytes(1, 'big'))
    file.write(encoded_text)

def _serialize_tex_axis(file, axis):
    file.write(int(axis.translate * 32).to_bytes(2, 'big'))
    file.write(int(axis.scale_log).to_bytes(1, 'big'))

    repeats = int(axis.repeats)

    if axis.mirror:
        repeats |= (1 << 15)

    file.write(repeats.to_bytes(2, 'big'))

def _serialize_tex(file, tex: material.Tex):
    if tex:
        _serialze_string(file, tex.rom_filename())
        file.write(tex.tmem_addr.to_bytes(2, 'big'))
        file.write(tex.palette.to_bytes(1, 'big'))
        _serialize_tex_axis(file, tex.s)
        _serialize_tex_axis(file, tex.t)

        file.write(struct.pack('>ff', tex.s.scroll, -tex.t.scroll))

        if tex.mag_filter == 'nearest':
            file.write((0).to_bytes(1, 'big'))
        else:
            file.write((1).to_bytes(1, 'big'))

        if tex.min_filter == 'nearest':
            file.write((0).to_bytes(1, 'big'))
        else:
            file.write((1).to_bytes(1, 'big'))
    else:
        file.write((0).to_bytes(1, 'big'))

def flags_for_material(mat: material.Material) -> int:
    flags = 0

    if mat.blend_mode and (mat.blend_mode.z_compare or mat.blend_mode.z_write):
        flags |= T3D_FLAG_DEPTH

    if mat.z_buffer:
        flags |= T3D_FLAG_DEPTH

    if mat.culling == 'front':
        flags |= T3D_FLAG_CULL_FRONT
    elif mat.culling == True:
        flags |= T3D_FLAG_CULL_BACK

    if mat.tex0:
        flags |= T3D_FLAG_TEXTURED

    if mat.combine_mode and mat.combine_mode.uses_shade():
        flags |= T3D_FLAG_SHADED

    return flags

def serialize_material_file(output, mat: material.Material, current_state: material.Material | None = None):
    output.write('MATR'.encode())

    _serialize_tex(output, mat.tex0)
    _serialize_tex(output, mat.tex1)

    force_cyc2 = False

    if mat.combine_mode and mat.combine_mode.cyc2:
        force_cyc2 = True

    if mat.blend_mode and mat.blend_mode.cyc2:
        force_cyc2 = True
    
    if mat.combine_mode:
        output.write(COMMAND_COMBINE.to_bytes(1, 'big'))
        _serialize_combine(output, mat.combine_mode, force_cyc2)

    if mat.blend_mode:
        # TODO check if combine is two cycle and 
        # modify the blend mode if it is
        output.write(COMMAND_BLEND.to_bytes(1, 'big'))
        _serialize_blend(output, mat.blend_mode, force_cyc2)
    
    if mat.env_color:
        output.write(COMMAND_ENV.to_bytes(1, 'big'))
        _serialize_color(output, mat.env_color)

    if mat.prim_color:
        output.write(COMMAND_PRIM.to_bytes(1, 'big'))
        _serialize_color(output, mat.prim_color)

    if mat.blend_color:
        output.write(COMMAND_BLEND_COLOR.to_bytes(1, 'big'))
        _serialize_color(output, mat.blend_color)

    if mat.uv_gen:
        output.write(COMMAND_UV_GEN.to_bytes(1, 'big'))
        output.write(UV_GEN[mat.uv_gen].to_bytes(1, 'big'))

    flags = flags_for_material(mat)

    if current_state:
        flags |= flags_for_material(current_state)

    output.write(COMMAND_FLAGS.to_bytes(1, 'big'))
    output.write(flags.to_bytes(2, 'big'))

    output.write(COMMAND_EOF.to_bytes(1, 'big'))


def serialize_material(filename, mat: material.Material):
    with open(filename, 'wb') as output:
        serialize_material_file(output, mat)