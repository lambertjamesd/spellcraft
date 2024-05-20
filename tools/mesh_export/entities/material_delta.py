
from . import material

def determine_tex_delta(start: material.Tex | None, end: material.Tex | None) -> material.Tex | None:
    if not start or not end:
        return end
    
    if start.filename == end.filename:
        return None
    
    return end

def determine_material_delta(start: material.Material, end: material.Material) -> material.Material:
    result = material.Material()

    if end.combine_mode and (not start.combine_mode or start.combine_mode != end.combine_mode):
        result.combine_mode = end.combine_mode
    
    if end.blend_color and (not start.blend_mode or start.blend_mode != end.blend_mode):
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

    if end.env_color and (not start.env_color or start.env_color != end.env_color):
        result.env_color = end.env_color

    if end.env_color and (not start.env_color or start.env_color != end.env_color):
        result.env_color = end.env_color

    if end.culling != None and (start.culling == None or start.culling != end.culling):
        result.culling = end.culling

    if end.z_buffer != None and (start.z_buffer == None or start.z_buffer != end.z_buffer):
        result.z_buffer = end.z_buffer

    return