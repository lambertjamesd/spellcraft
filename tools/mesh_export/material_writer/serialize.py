import material
import struct

COMMAND_EOF = 0
COMMAND_COMBINE = 1
COMMAND_ENV = 2
COMMAND_LIGHTING = 3

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

def _serialize_combine(file, combine: material.CombineMode):
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

    if combine.cyc2:
        a1 = COMB_A[combine.cyc2.a]
        b1 = COMB_B[combine.cyc2.b]
        c1 = COMB_C[combine.cyc2.c]
        d1 = COMB_D[combine.cyc2.d]

        aa1 = ACOMB_A[combine.cyc2.aa]
        ab1 = ACOMB_A[combine.cyc2.ab]
        ac1 = ACOMB_A[combine.cyc2.ac]
        ad1 = ACOMB_A[combine.cyc2.ad]

    file.write(struct.pack('>Q', \
        (a0 << 52) | (b0 << 28) | (c0 << 47) | (d0 << 15) | \
        (aa0 << 44) | (ab0 << 12) | (ac0 << 41) | (ad0 << 9) | \
        (a1 << 37) | (b1 << 24) | (c1 << 32) | (d1 << 6) |     \
        (aa1 << 21) | (ab1 << 3) | (ac1 << 18) | (ad1 << 0) \
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

def _serialize_tex(file, tex):
    if tex:
        _serialze_string(file, tex.filename)
        file.write(tex.tmem_addr.to_bytes(2, 'big'))
        file.write(tex.palette.to_bytes(1, 'big'))
        _serialize_tex_axis(file, tex.s)
        _serialize_tex_axis(file, tex.t)
    else:
        file.write((0).to_bytes(1, 'big'))


def serialize_material(filename, mat: material.Material):
    with open(filename, 'wb') as output:
        output.write('MATR'.encode())

        _serialize_tex(output, mat.tex0)
        
        if mat.combine_mode:
            output.write(COMMAND_COMBINE.to_bytes(1, 'big'))
            _serialize_combine(output, mat.combine_mode)
        
        if mat.env_color:
            output.write(COMMAND_ENV.to_bytes(1, 'big'))
            _serialize_color(output, mat.env_color)

        if not mat.lighting is None:
            output.write(COMMAND_LIGHTING.to_bytes(1, 'big'))
            output.write((1 if mat.lighting else 0).to_bytes(1, 'big'))

        output.write(COMMAND_EOF.to_bytes(1, 'big'))