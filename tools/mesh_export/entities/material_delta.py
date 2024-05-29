
from . import material

def determine_tex_delta(start: material.Tex | None, end: material.Tex | None) -> material.Tex | None:
    if not start or not end:
        return end
    
    if start.filename != end.filename:
        return end
    
    result = material.Tex()

    result.tmem_addr = end.tmem_addr
    result.palette = end.palette
    result.min_filter = end.min_filter
    result.mag_filter = end.mag_filter
    result.s = end.s
    result.t = end.t
    
    return result

def determine_material_delta(start: material.Material, end: material.Material) -> material.Material:
    result = material.Material()

    print(start, end)

    if end.combine_mode and (not start.combine_mode or start.combine_mode != end.combine_mode):
        result.combine_mode = end.combine_mode
    
    if end.blend_mode and (not start.blend_mode or start.blend_mode != end.blend_mode):
        result.blend_mode = end.blend_mode

    if end.env_color and (not start.env_color or start.env_color != end.env_color):
        result.env_color = end.env_color

    if end.prim_color and (not start.prim_color or start.prim_color != end.prim_color):
        result.prim_color = end.prim_color

    if end.blend_color and (not start.blend_color or start.blend_color != end.blend_color):
        result.blend_color = end.blend_color

    if end.lighting != None and (start.lighting == None or start.lighting != end.lighting):
        result.lighting = end.lighting

    result.tex0 = determine_tex_delta(start.tex0, end.tex0)
    result.tex1 = determine_tex_delta(start.tex1, end.tex1)

    if end.culling != None and (start.culling == None or start.culling != end.culling):
        result.culling = end.culling

    if end.z_buffer != None and (start.z_buffer == None or start.z_buffer != end.z_buffer):
        result.z_buffer = end.z_buffer

    return result

def determine_texture_cost(tex: material.Tex | None) -> float:
    if not tex:
        return 0

    result = 2

    if tex.filename:
        # todo, actually measure the texture size
        result += 2048 * 0.0072
    
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

    if mat.blend_mode:
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

    if mat.z_buffer != None:
        result += CHANGE_MODE

    return result