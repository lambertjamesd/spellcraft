import bpy
import mathutils
import math
import struct

fixed_sizes = {
    'float': 4,
    'int': 4,
    'uint64_t': 8,
    'uint32_t': 4,
    'uint16_t': 2,
    'uint8_t': 1,
    'int64_t': 8,
    'int32_t': 4,
    'int16_t': 2,
    'int8_t': 1,
    'struct Vector3': 12,
    'struct Vector2': 8,
    'struct Quaternion': 16,
}

struct_formats = {
    'float': 'f',
    'int': 'i',
    'uint64_t': 'Q',
    'uint32_t': 'I',
    'uint16_t': 'H',
    'uint8_t': 'B',
    'int64_t': 'q',
    'int32_t': 'i',
    'int16_t': 'h',
    'int8_t': 'b',
}

coordinate_convert = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')
coordinate_convert_invert = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

def get_transform(obj: bpy.types.Object) -> mathutils.Matrix:
    return coordinate_convert_invert @ obj.matrix_world @ coordinate_convert

def write_obj(file, obj: bpy.types.Object, definition, field_name = None):
    if isinstance(definition, str):
        if definition == 'struct Vector3':
            if field_name == 'position':
                loc, rot, scale = get_transform(obj).decompose()
                file.write(struct.pack(">fff", loc.x, loc.y, loc.z))
                return
        if definition == 'struct Vector2':
            if field_name == 'rotation':
                loc, rot, scale = get_transform(obj).decompose()
                rotated_right = rot @ mathutils.Vector([1, 0, 0])

                final_right = mathutils.Vector([rotated_right.x, 0, rotated_right.z]).normalized()

                file.write(struct.pack(">ff", final_right.x, final_right.z))
                return

         
        raise Exception(f"unknown field type {definition} {field_name}")
    
    for child in definition.children:
        write_obj(file, obj, child.data_type, child.name)


def obj_size(definition):
    if isinstance(definition, str):
        if not definition in fixed_sizes:
            raise Exception(f"{definition} is not a known size")
        return fixed_sizes[definition]
    
    result = 0

    for child in definition.children:
        result += obj_size(child.data_type)

    return result