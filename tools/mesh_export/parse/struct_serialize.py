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

def get_value(obj: bpy.types.Object, key: str, default_value):
    if key in obj:
        return obj[key]
    
    if key in obj.data:
        return obj.data[key]
    
    return default_value

def get_transform(obj: bpy.types.Object) -> mathutils.Matrix:
    return coordinate_convert_invert @ obj.matrix_world @ coordinate_convert

def write_obj(file, obj: bpy.types.Object, definition, enums, field_name = None):
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
        if definition == 'float':
            value = get_value(obj, field_name, 0)
            file.write(struct.pack(">f", value))
            return
        if definition in struct_formats:
            value = get_value(obj, field_name, 0)

            if value == True:
                value = 1

            if value == False:
                value = 0

            file.write(struct.pack(">" + struct_formats[definition], value))
            return
        if definition in enums:
            value = get_value(obj, field_name, None)

            if value == None:
                value = 0
            else:
                value = enums[definition].str_to_int(value)

            file.write(struct.pack(">I", value))
            return
         
        raise Exception(f"unknown field type '{definition}' {field_name}")
    
    for child in definition.children:
        write_obj(file, obj, child.data_type, enums, child.name)


def obj_size(definition, enums):
    if isinstance(definition, str):
        if definition in enums:
            return 4

        if not definition in fixed_sizes:
            raise Exception(f"{definition} is not a known size")
        return fixed_sizes[definition]
    
    result = 0

    for child in definition.children:
        result += obj_size(child.data_type, enums)

    return result