import bpy
import mathutils
import math
import struct

from . import struct_parse

class SerializeContext():
    def __init__(self, enums):
        self.enums = enums
        self._strings: dict[str, int] = {}
        self._string_data: list[bytes] = []
        self._current_offset = 0

    def get_string_offset(self, value: str):
        if value in self._strings:
            return self._strings[value]
        
        result = self._current_offset

        self._strings[value] = result
        string_data = value.encode() + bytes(1)
        self._string_data.append(string_data)
        self._current_offset += len(string_data)

        return result
    
    def search_enums(self, value: str):
        for single_enum in self.enums.values():
            if single_enum.is_defined(value):
                return single_enum.str_to_int(value)

        raise Exception(f'{value} is not found in any enum value')
    
    def write_strings(self, file):
        all_bytes = b''.join(self._string_data)
        file.write(struct.pack('>H', len(all_bytes)))
        file.write(all_bytes)


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

fixed_alignments = {
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
    'struct Vector3': 4,
    'struct Vector2': 4,
    'struct Quaternion': 4,
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

def _get_value(obj: bpy.types.Object, key: str, default_value):
    if key in obj:
        return obj[key]
    
    if key in obj.data:
        return obj.data[key]
    
    return default_value

def _get_transform(obj: bpy.types.Object) -> mathutils.Matrix:
    return coordinate_convert_invert @ obj.matrix_world @ coordinate_convert

def layout_strings(obj: bpy.types.Object, definition, context: SerializeContext, field_name = None):
    if isinstance(definition, struct_parse.PointerType):
        if definition.sub_type == 'char':
            context.get_string_offset(str(_get_value(obj, field_name, "")))

    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            layout_strings(obj, child.data_type, context, child.name)

def write_vector3_position(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    file.write(struct.pack(">fff", loc.x, loc.y, loc.z))


def write_vector2_rotation(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    rotated_right = rot @ mathutils.Vector([1, 0, 0])

    final_right = mathutils.Vector([rotated_right.x, 0, rotated_right.z]).normalized()

    file.write(struct.pack(">ff", final_right.x, -final_right.z))


def write_obj(file, obj: bpy.types.Object, definition, context: SerializeContext, field_name = None):
    if isinstance(definition, str):
        if definition == 'struct Vector3':
            if field_name == 'position':
                write_vector3_position(file, obj)
                return
        if definition == 'struct Vector2':
            if field_name == 'rotation':
                write_vector2_rotation(file, obj)
                return
        if definition == 'float':
            value = _get_value(obj, field_name, 0)
            file.write(struct.pack(">f", value))
            return
        if definition in struct_formats:
            value = _get_value(obj, field_name, 0)

            if value == True:
                value = 1

            if value == False:
                value = 0

            if isinstance(value, str):
                value = context.search_enums(value)

            file.write(struct.pack(">" + struct_formats[definition], value))
            return
        if definition in context.enums:
            value = _get_value(obj, field_name, None)

            if value == None:
                value = 0
            else:
                value = context.enums[definition].str_to_int(value)

            file.write(struct.pack(">I", value))
            return
         
        raise Exception(f"unknown field type '{definition}' {field_name}")
    
    if isinstance(definition, struct_parse.PointerType):
        if definition.sub_type == 'char':
            file.write(struct.pack(">I", context.get_string_offset(str(_get_value(obj, field_name, "")))))
            return
        
        raise Exception(f"unknown field type '{definition}' {field_name}")
    
    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            write_obj(file, obj, child.data_type, context, child.name)

TYPE_ID_STR = 0

class TypeLocation():
    def __init__(self, type_id: int, offset: int):
        self.type_id: int = type_id
        self.offset: int = offset

def _apply_alignment(current_offset: int, alignment: int) -> int:
    return (current_offset + alignment - 1) & ~(alignment - 1)

def obj_gather_types(definition, type_locations: list[TypeLocation], context: SerializeContext, current_offset: int = 0) -> tuple[int, int]:
    if isinstance(definition, str):
        if definition in context.enums:
            return _apply_alignment(current_offset, 4) + 4, 4

        if not definition in fixed_sizes:
            raise Exception(f"{definition} is not a known size")
        
        alignment = fixed_alignments[definition]

        return _apply_alignment(current_offset, alignment) + fixed_sizes[definition], alignment
    
    if isinstance(definition, struct_parse.PointerType):
        location = _apply_alignment(current_offset, 4)

        if definition.sub_type == 'char':
            type_locations.append(TypeLocation(TYPE_ID_STR, location))

        return location + 4, 4
    
    if isinstance(definition, struct_parse.StructureInfo):
        struct_alignment = 1

        for child in definition.children:
            current_offset, sub_alignment = obj_gather_types(child.data_type, type_locations, context, current_offset = current_offset)
            struct_alignment = max(struct_alignment, sub_alignment)

        return _apply_alignment(current_offset, struct_alignment), struct_alignment
    
    raise Exception(f'Unknown type {definition}')