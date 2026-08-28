import bpy
import mathutils
import math
import struct
import os.path
import os
from bpy.path import abspath

from . import struct_parse
from . import line_mesh_builder

def get_scene_resource(name: str) -> str:
    base = os.path.join(os.getcwd(), 'assets')
    library_location = bpy.data.filepath

    if not library_location.startswith(base):
        raise Exception(f'could not get rom path to {library_location} with base path {base}')

    return f'rom:{os.path.splitext(library_location[len(base):])[0]}_{name}'


def get_rom_path(library_path: str, new_suffix: str) -> str:
    base = os.path.join(os.getcwd(), 'assets')
    library_location = os.path.normpath(abspath(library_path))

    if not library_location.startswith(base):
        raise Exception(f'could not get rom path to {library_location} with base path {base}')

    return f'rom:{os.path.splitext(library_location[len(base):])[0]}{new_suffix}'

class SerializeContext():
    def __init__(self, enums):
        self.enums = enums
        self._strings: dict[str, int] = {}
        self._line_meshes: dict[bpy.types.Object, int] = {}
        self._string_data: list[bytes] = []
        self._current_offset = 0
        self._did_write = False
        self._obj_spawner_mapping: dict[str, int] = {}
        self._mesh_exports: set[bpy.types.Object] = set()

    def get_string_offset(self, value: str):
        if value in self._strings:
            return self._strings[value]
        
        if self._did_write:
            raise Exception(f'tried to layout string {value} after bytes were written')
        
        result = self._current_offset

        self._strings[value] = result
        string_data = value.encode() + bytes(1)
        self._string_data.append(string_data)
        self._current_offset += len(string_data)

        return result
    
    def get_line_mesh_offset(self, value: bpy.types.Object):
        if value in self._line_meshes:
            return self._line_meshes[value]
        
        if self._did_write:
            raise Exception(f'tried to layout line mesh {value} after bytes were written')
        
        result = self._current_offset

        self._line_meshes[value] = result
        mesh_data = line_mesh_builder.build_line_mesh(value, coordinate_convert_invert @ value.matrix_world)
        self._string_data.append(mesh_data)
        self._current_offset += len(mesh_data)

        return result
    
    
    def search_enums(self, value: str):
        for single_enum in self.enums.values():
            if single_enum.is_defined(value):
                return single_enum.str_to_int(value)

        raise Exception(f'{value} is not found in any enum value')

    def get_bytes(self) -> bytes:
        self._did_write = True

        return b''.join(self._string_data)
    
    def write_strings(self, file):
        all_bytes = self.get_bytes()
        file.write(struct.pack('>H', len(all_bytes)))
        file.write(all_bytes)

    def add_object_mapping(self, obj_name: str, room_index: int, entity_index: int):
        self._obj_spawner_mapping[obj_name] = (room_index << 16) | entity_index

    def get_spawner_id(self, obj_name: str) -> int:
        if obj_name in self._obj_spawner_mapping:
            return self._obj_spawner_mapping[obj_name]
        
        return 0xFFFFFFFF

    def add_mesh_export(self, mesh: bpy.types.Object):
        self._mesh_exports.add(mesh)

    def get_meshes_to_export(self) -> list[bpy.types.Object]:
        return list(self._mesh_exports)


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
    'collectable_sub_type': 4,
    'room_id': 2,
    'bool': 1,
    'boolean_variable': 2,
    'integer_variable': 2,
    'any_variable': 2,
    'line_mesh_data_ref': 4,
    'entity_spawner': 4,
    'collider_shape_t': 28,
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
    'collectable_sub_type': 4,
    'room_id': 2,
    'bool': 1,
    'boolean_variable': 2,
    'integer_variable': 2,
    'any_variable': 2,
    'line_mesh_data_ref': 4,
    'entity_spawner': 4,
    'collider_shape_t': 4,
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
    'collectable_sub_type': 'I',
    'room_id': 'H',
    'bool': 'B',
    'boolean_variable': 'H',
    'integer_variable': 'H',
    'any_variable': 'H',
}

struct_format_defaults = {
    'boolean_variable': 0xFFFF,
    'integer_variable': 0xFFFF,
    'any_variable': 0xFFFF,
}

_string_aliases = {
    'script_location',
    'scene_entry_point',
    'mesh_location',
}

SENSOR_SIZE = 36

def _is_string_type(definition):
    return isinstance(definition, struct_parse.PointerType) and definition.sub_type == 'char' or definition in _string_aliases

def _get_string_value(obj: bpy.types.Object, definition, field_name: str | None, context: SerializeContext) -> str | None:
    if not _is_string_type(definition):
        return None

    if definition == 'mesh_location':
        obj_name = str(get_value(obj, field_name, ""))

        to_extract = obj

        if obj_name.startswith('obj '):
            to_extract = bpy.data.objects[obj_name[len('obj '):]]

        mesh_obj = to_extract.data

        if not isinstance(mesh_obj, bpy.types.Mesh):
            return ''

        if mesh_obj.library:
            return get_rom_path(mesh_obj.library.filepath, '.tmesh')
        else:
            context.add_mesh_export(obj)
            return get_scene_resource(mesh_obj.name + '.tmesh')

    return str(get_value(obj, field_name, ""))

coordinate_convert = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')
coordinate_convert_invert = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

def get_value(obj: bpy.types.Object, key: str | None, default_value):
    if not key:
        return default_value

    if key in obj:
        return obj[key]
    
    if obj.data and key in obj.data:
        return obj.data[key]
    
    return default_value

def _get_transform(obj: bpy.types.Object) -> mathutils.Matrix:
    if obj.type == 'CAMERA':
        return coordinate_convert_invert @ obj.matrix_world
    return coordinate_convert_invert @ obj.matrix_world @ coordinate_convert

def get_position(obj: bpy.types.Object) -> mathutils.Vector:
    loc, rot, scale = _get_transform(obj).decompose()
    return loc

def get_scale(obj: bpy.types.Object) -> mathutils.Vector:
    loc, rot, scale = _get_transform(obj).decompose()
    return scale

def layout_strings(obj: bpy.types.Object, definition, context: SerializeContext, field_name = None):
    str_value = _get_string_value(obj, definition, field_name, context)

    if str_value != None:
        context.get_string_offset(str_value)

    if definition == "line_mesh_data_ref":
        context.get_line_mesh_offset(obj)

    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            layout_strings(obj, child.data_type, context, child.name)

def write_vector3_position(file, obj: bpy.types.Object):
    loc = get_position(obj)
    file.write(struct.pack(">fff", loc.x, loc.y, loc.z))

def write_vector3_scale(file, obj: bpy.types.Object):
    scale = get_scale(obj)
    file.write(struct.pack(">fff", scale.x, scale.y, scale.z))


def write_vector2_rotation(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    rotated_right = rot @ mathutils.Vector([1, 0, 0])

    final_right = mathutils.Vector([rotated_right.x, 0, rotated_right.z]).normalized()

    file.write(struct.pack(">ff", final_right.x, final_right.z))

def write_quaternion_rotation(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    file.write(struct.pack(">ffff", rot.x, rot.y, rot.z, rot.w))

def _apply_alignment(current_offset: int, alignment: int) -> int:
    return (current_offset + alignment - 1) & ~(alignment - 1)

def obj_determine_alignment(definition, context: SerializeContext) -> int:
    if _is_string_type(definition):
        return 4
    
    if isinstance(definition, str):
        if definition in fixed_alignments:
            return fixed_alignments[definition]
        if definition in context.enums:
            return 4
        raise Exception(f"{definition} doesn't have a known alignment")
    
    if isinstance(definition, struct_parse.PointerType):
        return 4
    
    if isinstance(definition, struct_parse.StructureInfo):
        if definition.align:
            return definition.align

        struct_alignment = 1
        for child in definition.children:
            struct_alignment = max(struct_alignment, obj_determine_alignment(child.data_type, context))
        definition.align = struct_alignment
        return struct_alignment
    
    raise Exception(f'Unknown type {definition}')

def _write_padding(file, offset: int, definition, context: SerializeContext) -> int:
    new_offset = _apply_alignment(offset, obj_determine_alignment(definition, context))

    if new_offset == offset:
        return offset

    for i in range(new_offset - offset):
        file.write(struct.pack('>B', 0))
    
    return new_offset

def _write_collider_def(file, obj: bpy.types.Object, collider_obj: bpy.types.Object) -> bool:
    relative_transform = obj.matrix_world.inverted() @ collider_obj.matrix_world

    if not isinstance(collider_obj.data, bpy.types.Mesh) or len(collider_obj.data.vertices) == 0:
        return False

    mesh = collider_obj.data

    min_pos = relative_transform @ mesh.vertices[0].co
    max_pos = min_pos

    for vtx in mesh.vertices:
        vtx_pos = relative_transform @ vtx.co
        min_pos = mathutils.Vector((
            min(vtx_pos.x, min_pos.x),
            min(vtx_pos.y, min_pos.y),
            min(vtx_pos.z, min_pos.z)
        ))
        max_pos = mathutils.Vector((
            max(vtx_pos.x, max_pos.x),
            max(vtx_pos.y, max_pos.y),
            max(vtx_pos.z, max_pos.z)
        ))

    half_size = (max_pos - min_pos) * 0.5
    center = (min_pos + max_pos) * 0.5

    file.write(struct.pack(
        ">Iffffff", 
        2, 
        half_size.x, half_size.z, half_size.y,
        center.x, center.z, -center.y
    ))
    return True


def write_obj(file, obj: bpy.types.Object, definition, context: SerializeContext, field_name: str | None = None, offset: int = 0) -> int:
    offset = _write_padding(file, offset, definition, context)

    str_value = _get_string_value(obj, definition, field_name, context)

    if str_value != None:
        file.write(struct.pack(">I", context.get_string_offset(str_value)))
        return offset + 4
    
    if isinstance(definition, str):
        if definition == 'struct Vector3':
            if field_name == 'position':
                write_vector3_position(file, obj)
                return offset + 12
            if field_name == 'scale':
                write_vector3_scale(file, obj)
                return offset + 12
            
            value = get_value(obj, field_name, 0)
            if isinstance(value, str):
                if value.startswith('obj '):
                    write_vector3_position(file, bpy.data.objects[value[len('obj '):]])
                else:
                    file.write(struct.pack('>fff', 0, 0, 0))
                return offset + 12
        elif definition == 'struct Vector2':
            if field_name == 'rotation':
                write_vector2_rotation(file, obj)
                return offset + 8
        elif definition == 'struct Quaternion':
            if field_name == 'rotation':
                write_quaternion_rotation(file, obj)
                return offset + 16
            
            value = get_value(obj, field_name, 0)
            if isinstance(value, str):
                if value.startswith('obj '):
                    write_quaternion_rotation(file, bpy.data.objects[value[len('obj '):]])
                else:
                    file.write(struct.pack('>ffff', 0, 0, 0, 1))
                return offset + 16
        elif definition == 'float':
            if field_name == 'scale':
                value = get_scale(obj).x
            elif field_name == 'fov' and obj.type == 'CAMERA':
                value = obj.data.angle_y * 180 / math.pi
            else:
                value = get_value(obj, field_name, 0)
            file.write(struct.pack(">f", value))
            return offset + 4
        elif definition in struct_formats:
            value = get_value(obj, field_name, struct_format_defaults[definition] if definition in struct_format_defaults else 0)

            if value == True:
                value = 1

            if value == False:
                value = 0

            if isinstance(value, str):
                value = context.search_enums(value)

            file.write(struct.pack(">" + struct_formats[definition], value))
            return offset + fixed_sizes[definition]
        elif definition in context.enums:
            value = get_value(obj, field_name, None)

            if value == None:
                value = 0
            else:
                value = context.enums[definition].str_to_int(value)

            file.write(struct.pack(">I", value))
            return offset + 4
        elif definition == 'line_mesh_data_ref':
            file.write(struct.pack(">I", context.get_line_mesh_offset(obj)))
            return offset + 4
        elif definition == 'entity_spawner':
            spawner_name = get_value(obj, field_name, '')
            file.write(struct.pack(">I", context.get_spawner_id(spawner_name)))
            return offset + 4
        elif definition == 'collider_shape_t':
            value = get_value(obj, field_name, 0)
            if not isinstance(value, str) or not _write_collider_def(file, obj, bpy.data.objects[value[len('obj '):]]):
                file.write(struct.pack(">Iffffff", 2, 0, 0, 0, 0, 0, 0))
            
            return offset + 28
         
        raise Exception(f"unknown field type '{definition}' {field_name}")
    
    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            offset = write_obj(file, obj, child.data_type, context, child.name, offset)

    if field_name == None:
        # add end of struct padding
        _write_padding(file, offset, definition, context)

TYPE_ID_STR = 0

class TypeLocation():
    def __init__(self, type_id: int, offset: int):
        self.type_id: int = type_id
        self.offset: int = offset

def _obj_gather_types(definition, context: SerializeContext, current_offset: int) -> int:
    current_offset = _apply_alignment(current_offset, obj_determine_alignment(definition, context))

    if _is_string_type(definition):
        return current_offset + 4
    
    if isinstance(definition, str):
        if definition in fixed_sizes:
            return current_offset + fixed_sizes[definition]

        if definition in context.enums:
            return current_offset + 4

        raise Exception(f"{definition} is not a known size")
    
    if isinstance(definition, struct_parse.PointerType):
        return current_offset + 4
    
    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            current_offset = _obj_gather_types(child.data_type, context, current_offset = current_offset)

        return current_offset
    
    raise Exception(f'Unknown type {definition}')

def obj_gather_types(definition, context: SerializeContext) -> int:
    result = _obj_gather_types(definition, context, 0)
    return _apply_alignment(result, obj_determine_alignment(definition, context))