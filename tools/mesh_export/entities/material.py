import bpy
import json
import os.path
import numbers
import sys

class Color():
    def __init__(self, r, g, b, a):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    def __eq__(self, value: object) -> bool:
        return False
    
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
        self.cyc1: CombineModeCycle = cyc1
        self.cyc2: CombineModeCycle = cyc2

    def __eq__(self, value: object) -> bool:
        return False

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
class BlendModeCycle():
    def __init__(self, a1, b1, a2, b2):
        self.a1 = a1
        self.b1 = b1
        self.a2 = a2
        self.b2 = b2

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
        return False

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
    
class Tex():
    def __init__(self):
        self.filename = None
        self.tmem_addr = 0
        self.palette = 0
        self.min_filter = "nearest"
        self.mag_filter = "nearest"
        self.s = TexAxis()
        self.t = TexAxis()
        self.sequenceLength: int = 0

    def rom_filename(self) -> str:
        return f"rom:{self.filename[len('assets'):-len('.png')]}.sprite"

    def __str__(self):
        return self.filename

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

    result.culling = _optional_boolean(json_data, 'culling', 'culling', None)
    result.z_buffer = _optional_boolean(json_data, 'zBuffer', 'zBuffer', None)

    result.vertex_gamma = _optional_number(json_data, 'vertexGamma', 'vertexGamma', 1)

    return result

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
    return Color(
        color_float_to_int(array[0]),
        color_float_to_int(array[1]),
        color_float_to_int(array[2]),
        color_float_to_int(array[3])
    )

def determine_material_from_nodes(mat: bpy.types.Material, result: Material):
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

        result.tex0 = Tex()
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
        result.combine_mode = CombineMode(
            CombineModeCycle(
                color_name, '0', 'SHADE', '0',
                color_name, '0', 'SHADE', '0'
            ),
            None
        )
    else:
        result.combine_mode = CombineMode(
            CombineModeCycle(
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
        result.blend_mode = BlendMode(BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'THRESHOLD'
        result.blend_color = Color(0, 0, 0, 128)
    elif mat.blend_method == 'BLEND':
        result.blend_mode = BlendMode(BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'), None)
        result.blend_mode.z_write = False
    elif mat.blend_method == 'HASHED':
        result.blend_mode = BlendMode(BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'NOISE'
    else:
        result.blend_mode = BlendMode(BlendModeCycle('IN', '0', 'IN', '1'), None)

    if 'decal' in mat and mat['decal']:
        result.blend_mode.z_mode = 'DECAL'
        result.blend_mode.z_write = False

def load_material_with_name(material_name: str, bpy_mat: bpy.types.Material):
    material_filename = f"assets/{material_name}.mat.json"

    if not material_name.startswith('materials/'):
        # embedded material
        material_object = Material()

        if bpy_mat.use_nodes:
            determine_material_from_nodes(bpy_mat, material_object)

        return material_object

    elif os.path.exists(material_filename):
        material_object = parse_material(material_filename)
        return material_object
    else:
        raise Exception(f"{material_filename} does not exist")