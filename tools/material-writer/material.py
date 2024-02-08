import json
import os.path
import numbers

class Color():
    def __init__(self, r, g, b, a):
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    
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

class CombineMode():
    def __init__(self, cyc1: CombineModeCycle, cyc2: CombineModeCycle):
        self.cyc1 = cyc1
        self.cyc2 = cyc2

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
class TexAxis():
    def __init__(self):
        self.translate = 0
        self.scale_log = 0
        self.repeats = 2048
        self.mirror = False
    
class Tex():
    def __init__(self):
        self.filename = None
        self.tmem_addr = 0
        self.palette = 0
        self.min_filter = "nearest"
        self.max_filter = "nearest"
        self.s = TexAxis()
        self.t = TexAxis()

class Material():
    def __init__(self):
        self.combine_mode = None
        self.blend_mode = None
        self.env_color = None
        self.prim_color = None
        self.lighting = None
        self.tex0 = None

    def __str__(self):
        return f"""Material:
    combine_mode = {self.combine_mode}
    blend_mode = {self.blend_mode}
    env_color = {self.env_color}
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
    into.translate = _optional_number(json_data, 'translate', into.translate)
    into.scale_log = _optional_number(json_data, 'scale_log', into.scale_log)
    into.repeats = _optional_number(json_data, 'repeats', into.repeats)
    into.mirror = _optional_boolean(json_data, 'mirror', False)

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

        if 'tmem_addr' in json_data:
            _check_is_int(json_data['tmem_addr'], f"{key_path}.tmem_addr", 0, 512)
            result.tmem_addr = json_data['tmem_addr']

        if 'min_filter' in json_data:
            _check_is_enum(json_data['min_filter'], f"{key_path}.min_filter", ['nearest', 'linear'])
            result.tmem_addr = json_data['min_filter']

        if 'max_filter' in json_data:
            _check_is_enum(json_data['max_filter'], f"{key_path}.max_filter", ['nearest', 'linear'])
            result.tmem_addr = json_data['max_filter']

        if 's' in json_data:
            _parse_tex_axis(json_data['s'], result.s, f"{key_path}.s")

        if 't' in json_data:
            _parse_tex_axis(json_data['t'], result.t, f"{key_path}.t")

    combined_path = os.path.join(os.path.dirname(relative_to), result.filename)
    combined_path = os.path.normpath(combined_path)

    if not os.path.exists(combined_path):
        raise Exception(f"The image file {combined_path} does not exist")
    
    if not combined_path.startswith('assets'):
        raise Exception(f"The image {combined_path} must be in the assets folder")
    
    if not combined_path.endswith('.png'):
        raise Exception(f"The image {combined_path} must be a png file")
    
    result.filename = f"rom:{combined_path[len('assets'):-len('.png')]}.sprite"

    return result


def parse_material(filename):
    if not os.path.exists(filename):
        raise Exception(f"The file {filename} does not exist")

    with open(filename, 'r') as json_file:
        json_data = json.load(json_file)

    result = Material()

    result.tex0 = _parse_tex(json_data['tex0'], 'tex0', filename) if 'tex0' in json_data else None

    _parse_combine_mode(result, json_data)
    result.env_color = _parse_color(json_data['envColor'], 'envColor') if 'envColor' in json_data else None

    result.lighting = json_data['lighting'] if 'lighting' in json_data else None

    return result