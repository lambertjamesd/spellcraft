import json
import os.path
import numbers
import struct

class Color():
    def __init__(self, r, g, b, a):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    def __eq__(self, value: object) -> bool:
        if not value:
            return False
        
        return self.r == value.r and self.g == value.g and self.b == value.b and self.a == value.a
    
    def __str__(self):
        return f"Color({self.r} {self.g} {self.b} {self.a})"

class CombineModeCycle():
    def __init__(self, a, b, c, d, aa, ab, ac, ad):
        self.a = a
        self.b = b
        self.c = c
        self.d = d

        self.aa = aa
        self.ab = ab
        self.ac = ac
        self.ad = ad

    def __str__(self):
        return f"({self.a} {self.b} {self.c} {self.d}) ({self.aa} {self.ab} {self.ac} {self.ad})"
    
    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineModeCycle):
            return False
        
        return self.a == value.a and self.b == value.b and self.c == value.c and self.d == value.d and \
            self.aa == value.aa and self.ab == value.ab and self.ac == value.ac and self.ad == value.ad

    def uses_shade(self):
        return self.a == 'SHADE' or self.b == 'SHADE' or self.c == 'SHADE' or self.c == 'SHADE_ALPHA' or self.d == 'SHADE' or \
            self.aa == 'SHADE' or self.ab == 'SHADE' or self.ac == 'SHADE' or self.ad == 'SHADE'

class CombineMode():
    def __init__(self, cyc1: CombineModeCycle, cyc2: CombineModeCycle):
        self.cyc1: CombineModeCycle = cyc1
        self.cyc2: CombineModeCycle = cyc2

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineMode):
            return False

        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
    def uses_shade(self) -> bool:
        return self.cyc1.uses_shade() or (self.cyc2 and self.cyc2.uses_shade())
    
class BlendModeCycle():
    def __init__(self, a1, b1, a2, b2):
        self.a1 = a1
        self.b1 = b1
        self.a2 = a2
        self.b2 = b2

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, BlendModeCycle):
            return False
        
        return self.a1 == value.a1 and self.b1 == value.b1 and self.a2 == value.a2 and self.b2 == value.b2

    def needs_read(self):
        return self.a1 == 'MEMORY' or self.a2 == 'MEMORY' or self.b1 == 'MEMORY_CVG' or self.b2 == 'MEMORY_CVG'
    
    def __str__(self):
        return f"({self.a1} {self.b1} {self.a2} {self.b2})"

class BlendMode():
    def __init__(self, cyc1: BlendModeCycle, cyc2: BlendModeCycle):
        self.cyc1: BlendModeCycle = cyc1
        self.cyc2: BlendModeCycle = cyc2
        self.alpha_compare = 'NONE'
        self.z_mode = 'OPAQUE'
        self.z_write = True
        self.z_compare = True

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, BlendMode):
            return False
        
        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2 and self.alpha_compare == value.alpha_compare and \
            self.z_mode == value.z_mode and self.z_write == value.z_write and self.z_compare == value.z_compare

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
class Palette():
    def __init__(self):
        self.colors: list[Color] = []
    
class TexAxis():
    def __init__(self):
        self.translate = 0
        self.scale_log = 0
        self.repeats = 2048
        self.mirror = False
        self.scroll = 0
    
class Tex():
    def __init__(self):
        self.filename: str | None = None
        self.tmem_addr = 0
        self.palette = 0
        self.min_filter = "nearest"
        self.mag_filter = "nearest"
        self.s = TexAxis()
        self.t = TexAxis()
        self.sequenceLength: int = 0

        self._size: tuple[int, int] | None = None

    def rom_filename(self) -> str:
        return f"rom:{self.filename[len('assets'):-len('.png')]}.sprite"

    def get_image_size(self) -> tuple[int, int]:
        if self._size:
            return self._size

        if not self.filename:
            return (128, 128)

        if self.filename.endswith('.png'):
            with open(self.filename, 'rb') as file:
                file.seek(16, 0)
                # the png specification states the header
                # should be the first chunk so the iamge
                # size should always be at this location
                w, = struct.unpack('>I', file.read(4))
                h, = struct.unpack('>I', file.read(4))

                self._size = (w, h)

                return self._size
            
        raise Exception(f"{self.filename} is not a supported file type")
        
    def __str__(self):
        if self.filename:
            return self.filename
        
        return "NULL"

class Material():
    def __init__(self):
        self.combine_mode: CombineMode = None
        self.blend_mode: BlendMode = None
        self.env_color: Color = None
        self.prim_color: Color | None = None
        self.blend_color: Color | None = None
        self.lighting: bool | None = None
        self.tex0: Tex | None = None
        self.tex1: Tex | None = None
        self.culling: bool | None = None
        self.z_buffer: bool | None = None
        self.vertex_gamma: float = 0.454545
        self.uv_gen: str | None = None

    def get_image_size(self) -> tuple[int, int]:
        if self.tex0:
            return self.tex0.get_image_size()
        
        return 128, 128

    def is_empty(self):
        return self.combine_mode == None and self.blend_color == None and self.env_color == None and self.prim_color == None and \
            self.blend_color == None and self.lighting == None and self.tex0 == None and self.tex1 == None and self.culling == None and \
            self.z_buffer == None

    def __str__(self):
        return f"""Material:
    combine_mode = {self.combine_mode}
    blend_mode = {self.blend_mode}
    env_color = {self.env_color}
    prim_color = {self.prim_color}
    blend_color = {self.blend_color}
    culling = {self.culling}
    lighting = {self.lighting}
    tex0 = {self.tex0}
    tex1 = {self.tex1}
"""

def _check_is_enum(value, key_path, enum_list):
    if value in enum_list:
        return
    
    raise Exception(f"{key_path} is not a valid value. got '{value}' expected {', '.join(enum_list)}")

def _check_is_int(value, key_path, min, max):
    if not isinstance(value, int) or value < min or value > max:
        raise Exception(f"{key_path} must be an int between {min} and {max}")

def _check_is_number(value, key_path):
    if not isinstance(value, numbers.Real):
        raise Exception(f"{key_path} must be a number")
    
def _optional_number(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_number(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_boolean(value, key_path):
    if not isinstance(value, bool):
        raise Exception(f"{key_path} must be a boolean")
    
def _optional_boolean(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_boolean(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_string(value, key_path):
    if not isinstance(value, str):
        raise Exception(f"{key_path} must be a string")
    
def _optional_string(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_string(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _parse_combine_mode_cycle(json_data, key_path):
    color = json_data['color']

    if not color:
        raise Exception(f"{key_path}.color must be defined")
    
    if not isinstance(color, list) or len(color) != 4:
        raise Exception(f"{key_path}.color must be an array of length 4")
    
    _check_is_enum(
        color[0], 
        f'{key_path}.color[0]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "NOISE",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        color[1], 
        f'{key_path}.color[1]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "CENTER",
            "K4",
            "0"
        ],
    )

    _check_is_enum(
        color[2], 
        f'{key_path}.color[2]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "SCALE",
            "COMBINED_ALPHA",
            "TEX0_ALPHA",
            "TEX1_ALPHA",
            "PRIM_ALPHA",
            "SHADE_ALPHA",
            "ENV_ALPHA",
            "LOD_FRACTION",
            "PRIM_LOD_FRAC",
            "K5",
            "0"
        ],
    )

    _check_is_enum(
        color[3], 
        f'{key_path}.color[3]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )
    
    alpha = json_data['alpha']

    if not alpha:
        alpha = ["0", "0", "0", "1"]

    if not isinstance(alpha, list) or len(alpha) != 4:
        raise Exception(f"{key_path}.alpha must be an array of length 4")

    _check_is_enum(
        alpha[0], 
        f'{key_path}.alpha[0]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        alpha[1], 
        f'{key_path}.alpha[1]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        alpha[2], 
        f'{key_path}.alpha[2]', 
        [
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "LOD_FRACTION",
            "PRIM_LOD_FRAC",
            "0"
        ],
    )

    _check_is_enum(
        alpha[3], 
        f'{key_path}.alpha[3]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    return CombineModeCycle(
        color[0],
        color[1],
        color[2],
        color[3],

        alpha[0],
        alpha[1],
        alpha[2],
        alpha[3],
    )

def _parse_combine_mode(result: Material, json_data):
    if not 'combineMode' in json_data:
        return
    
    combine_mode = json_data['combineMode']
    
    if isinstance(combine_mode, list):
        if len(combine_mode) > 2:
            raise Exception('combineMode can only have up to two cycles')

        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode[0], 'combineMode[0]'),
            _parse_combine_mode_cycle(combine_mode[1], 'combineMode[1]') if len(combine_mode) == 2 else None,
        )
    else:
        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode, 'combineMode'),
            None
        )

def _parse_blend_mode_cycle(json_data, key_path):
    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} must be an array of length 4")

    _check_is_enum(
        json_data[0],
        f"{key_path}[0]",
        ["IN", "MEMORY", "BLEND", "FOG"]
    )

    _check_is_enum(
        json_data[1],
        f"{key_path}[1]",
        ["IN_A", "FOG_A", "SHADE_A", "0", "ZERO"]
    )

    _check_is_enum(
        json_data[2],
        f"{key_path}[2]",
        ["IN", "MEMORY", "BLEND", "FOG"]
    )

    _check_is_enum(
        json_data[3],
        f"{key_path}[3]",
        ["INV_MUX_A", "MEMORY_CVG", "ONE", "1", "ZERO", "0"]
    )

    return BlendModeCycle(json_data[0], json_data[1], json_data[2], json_data[3])

def _parse_blend_mode(result: Material, json_data):
    if not 'blendMode' in json_data:
        return
    
    blend_mode = json_data['blendMode']

    if blend_mode == 'OPAQUE':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "0", "IN", "1"), None)
        return

    if blend_mode == 'TRANSPARENT':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "IN_A", "MEMORY", "INV_MUX_A"), None)
        result.blend_mode.z_write = False
        result.blend_mode.z_mode = 'TRANSPARENT'
        return
    
    if blend_mode == 'ALPHA_CLIP':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.alpha_compare = 'THRESHOLD'
        result.blend_color = Color(0, 0, 0, 128)
        return
    
    if blend_mode == 'ADD':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "IN_A", "MEMORY", "1"), None)
        result.blend_mode.z_write = False
        result.blend_mode.z_mode = 'TRANSPARENT'
        return

    if not 'cyc1' in blend_mode:
        raise Exception(f"blendMode must have at least 1 cycle")
    
    cyc1 = _parse_blend_mode_cycle(blend_mode['cyc1'], 'blendMode.cyc1')
    cyc2 = None

    if 'cyc2' in blend_mode:
        cyc2 = _parse_blend_mode_cycle(blend_mode['cyc2'], 'blendMode.cyc2')

    result.blend_mode = BlendMode(cyc1, cyc2)

    if 'zMode' in blend_mode:
        _check_is_enum(blend_mode['zMode'], 'blendMode.zMode', ['OPAQUE', 'INTER', 'TRANSPARENT', 'DECAL'])
        result.blend_mode.z_mode = blend_mode['zMode']

    if 'zWrite' in blend_mode:
        _check_is_boolean(blend_mode['zWrite'], 'blendMode.zWrite')
        result.blend_mode.z_write = blend_mode['zWrite']

    if 'zCompare' in blend_mode:
        _check_is_boolean(blend_mode['zCompare'], 'blendMode.zCompare')
        result.blend_mode.z_compare = blend_mode['zCompare']

    if 'alphaCompare' in blend_mode:
        _check_is_enum(blend_mode['alphaCompare'], 'blendMode.alphaCompare', ['NONE', 'THRESHOLD', 'NOISE'])


def _parse_color(json_data, key_path):
    if not json_data:
        return None
    
    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} should be an array of 4 number [0,255]")
    
    _check_is_int(json_data[0], f"{key_path}[0]", 0, 255)
    _check_is_int(json_data[1], f"{key_path}[1]", 0, 255)
    _check_is_int(json_data[2], f"{key_path}[2]", 0, 255)
    _check_is_int(json_data[3], f"{key_path}[3]", 0, 255)

    return Color(json_data[0], json_data[1], json_data[2], json_data[3])

def _parse_tex_axis(json_data, into: TexAxis, key_path):
    into.translate = _optional_number(json_data, 'translate', key_path, into.translate)
    into.scale_log = _optional_number(json_data, 'scale_log', key_path, into.scale_log)
    into.repeats = _optional_number(json_data, 'repeats', key_path, into.repeats)
    into.mirror = _optional_boolean(json_data, 'mirror', key_path, False)
    into.scroll = _optional_number(json_data, 'scroll', key_path, into.scroll)

def _parse_tex(json_data, key_path, relative_to):
    if not json_data:
        return None
    
    result = Tex()

    if isinstance(json_data, str):
        result.filename = json_data
    else:
        if not 'filename' in json_data:
            raise Exception(f"{key_path}.filename must be defined")
        
        _check_is_string(json_data['filename'], f"{key_path}.filename")
        result.filename = json_data['filename']

        if 'tmemAddr' in json_data:
            _check_is_int(json_data['tmemAddr'], f"{key_path}.tmemAddr", 0, 4096)
            result.tmem_addr = json_data['tmemAddr']

        if 'minFilter' in json_data:
            _check_is_enum(json_data['minFilter'], f"{key_path}.minFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['minFilter']

        if 'magFilter' in json_data:
            _check_is_enum(json_data['magFilter'], f"{key_path}.magFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['magFilter']

        if 's' in json_data:
            _parse_tex_axis(json_data['s'], result.s, f"{key_path}.s")

        if 't' in json_data:
            _parse_tex_axis(json_data['t'], result.t, f"{key_path}.t")

        if 'sequenceLength' in json_data:
            result.sequence_length = _check_is_int(json_data['sequenceLength'], f"{key_path}.sequenceLength", 0, 64)

    combined_path = os.path.join(os.path.dirname(relative_to), result.filename)
    combined_path = os.path.normpath(combined_path)

    if not os.path.exists(combined_path):
        raise Exception(f"The image file {combined_path} does not exist")
    
    if not combined_path.startswith('assets'):
        raise Exception(f"The image {combined_path} must be in the assets folder")
    
    if not combined_path.endswith('.png'):
        raise Exception(f"The image {combined_path} must be a png file")
    
    result.filename = combined_path

    return result


def parse_material(filename: str):
    if not os.path.exists(filename):
        raise Exception(f"The file {filename} does not exist")

    with open(filename, 'r') as json_file:
        json_data = json.load(json_file)

    result = Material()

    result.tex0 = _parse_tex(json_data['tex0'], 'tex0', filename) if 'tex0' in json_data else None
    result.tex1 = _parse_tex(json_data['tex1'], 'tex1', filename) if 'tex1' in json_data else None

    _parse_combine_mode(result, json_data)
    _parse_blend_mode(result, json_data)
    result.env_color = _parse_color(json_data['envColor'], 'envColor') if 'envColor' in json_data else None
    result.prim_color = _parse_color(json_data['primColor'], 'primColor') if 'primColor' in json_data else None
    result.blend_color = _parse_color(json_data['blendColor'], 'blendColor') if 'blendColor' in json_data else None

    result.lighting = json_data['lighting'] if 'lighting' in json_data else None

    result.culling = _optional_boolean(json_data, 'culling', 'culling', True)
    result.z_buffer = _optional_boolean(json_data, 'zBuffer', 'zBuffer', True)

    result.vertex_gamma = _optional_number(json_data, 'vertexGamma', 'vertexGamma', 1)

    result.uv_gen = _optional_string(json_data, 'uvGen', 'uvGen', 'none')

    return result
