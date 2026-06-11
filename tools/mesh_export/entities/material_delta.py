
from . import material

def determine_tex_delta(start: material.Tex | None, end: material.Tex | None, st0_filename: str | None = None, et0_filename: str | None = None) -> material.Tex | None:
    if not start or not end:
        return end
    
    accept_last_image_data = True
    
    if start.filename == None and end.filename == None:
        accept_last_image_data = st0_filename == et0_filename
    
    if start == end and accept_last_image_data:
        return None
    
    result = end.copy()

    if start.filename == end.filename:
        result.filename = None

    if start.palette_data == end.palette_data and accept_last_image_data:
        result.palette_data = None

    if start.frames == end.frames:
        result.frames = None

    return result

def determine_fog_delta(start: material.Fog | None, end: material.Fog | None) -> material.Fog | None:
    if not start or not end:
        return end

    if start.enabled == end.enabled:
        return None

    result = material.Fog()

    result.enabled = end.enabled

    return result

def determine_material_delta(start: material.Material, end: material.Material) -> material.Material:
    result = material.Material()

    if end.combine_mode and (not start.combine_mode or start.combine_mode != end.combine_mode):
        result.combine_mode = end.combine_mode
    
    if end.other_modes and (not start.other_modes or start.other_modes != end.other_modes):
        result.other_modes = end.other_modes

    if end.other_modes:
        result.other_modes = end.other_modes

    if end.env_color and (not start.env_color or start.env_color != end.env_color):
        result.env_color = end.env_color

    if end.prim_color and (not start.prim_color or start.prim_color != end.prim_color):
        result.prim_color = end.prim_color

    if end.blend_color and (not start.blend_color or start.blend_color != end.blend_color):
        result.blend_color = end.blend_color

    if end.lighting != None and (start.lighting == None or start.lighting != end.lighting):
        result.lighting = end.lighting

    result.tex0 = determine_tex_delta(start.tex0, end.tex0)
    result.tex1 = determine_tex_delta(
        start.tex1, 
        end.tex1,
        start.tex0.filename if start.tex0 else None,
        end.tex0.filename if end.tex0 else None
    )

    if end.culling != None and (start.culling == None or start.culling != end.culling):
        result.culling = end.culling

    if end.vtx_effect != None and (start.vtx_effect == None or start.vtx_effect != end.vtx_effect):
        result.vtx_effect = end.vtx_effect

    # this is a mystery to me
    # result.fog = determine_fog_delta(start.fog, end.fog)
    result.fog = end.fog

    if end.flags and (not start.flags or start.flags != end.flags):
        result.flags = end.flags
        

    return result

def determine_texture_cost(tex: material.Tex | None) -> float:
    if not tex:
        return 0

    result = 2

    if tex.filename or tex.frames:
        result += tex.byte_size() * 0.0072
    elif tex.palette_data:
        result += len(tex.palette_data) * 2 * 0.0072
    
    return 0

# todo base these timings on tiny3d, not lib ultra
COMBINE_MODE_TIME = 0.395
BLEND_MODE_TIME = 0.751
COLOR_CHANGE_TIME = 0.395
CHANGE_MODE = 0.7

def determine_material_cost(mat: material.Material) -> float:
    result = 0

    if mat.combine_mode:
        result += COMBINE_MODE_TIME

    if mat.other_modes:
        result += BLEND_MODE_TIME

    if mat.env_color:
        result += COLOR_CHANGE_TIME

    if mat.prim_color:
        result += COLOR_CHANGE_TIME

    if mat.blend_color:
        result += COLOR_CHANGE_TIME

    if mat.lighting != None:
        result += CHANGE_MODE

    result += determine_texture_cost(mat.tex0)
    result += determine_texture_cost(mat.tex1)

    if mat.culling != None:
        result += CHANGE_MODE

    if mat.fog:
        result += CHANGE_MODE

    if mat.flags:
        result += CHANGE_MODE

    return result

def apply_tex_delta(delta: material.Tex, into: material.Tex | None) -> material.Tex:
    if not into:
        return delta
    
    result = into.copy()
    
    if delta.filename:
        result.filename = delta.filename
        result.frames = None

    if delta.palette_data:
        result.palette_data = delta.palette_data

    if delta.frames:
        result.frames = delta.frames
        result.filename = None

    result.tmem_addr = delta.tmem_addr
    result.palette = delta.palette
    result.min_filter = delta.min_filter
    result.mag_filter = delta.mag_filter
    result.s = delta.s
    result.t = delta.t
    result.fmt = delta.fmt
    result.width = delta.width
    result.height = delta.height

    if delta.byte_size() + delta.tmem_addr > 2048:
        result.palette_data = None
    
    return result

def apply_material_delta(delta: material.Material, into: material.Material):
    if delta.combine_mode != None:
        into.combine_mode = delta.combine_mode

    if delta.other_modes != None:
        into.other_modes = delta.other_modes

    if delta.env_color != None:
        into.env_color = delta.env_color

    if delta.prim_color != None:
        into.prim_color = delta.prim_color

    if delta.blend_color != None:
        into.blend_color = delta.blend_color

    if delta.lighting != None:
        into.lighting = delta.lighting

    if delta.tex0 != None:
        into.tex0 = apply_tex_delta(delta.tex0, into.tex0)

    if delta.tex1 != None:
        into.tex1 = apply_tex_delta(delta.tex1, into.tex1)

    if delta.culling != None:
        into.culling = delta.culling

    if delta.vtx_effect != None:
        into.vtx_effect = delta.vtx_effect

    if delta.fog != None:
        into.fog = delta.fog

    if delta.flags != None:
        into.flags = delta.flags
