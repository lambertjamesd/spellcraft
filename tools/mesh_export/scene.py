import bpy
import sys
import os
import mathutils
import math
import struct

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.mesh_collider
import entities.tiny3d_mesh_writer
import entities.mesh_optimizer
import entities.material_extract
import entities.entry_point
from entities.entities import ObjectEntry
import entities.overworld
import parse.struct_parse
import parse.struct_serialize
import cutscene.expresion_generator
import cutscene.parser
import cutscene.variable_layout
import entities.camera_animation
import entities.room

class StaticEntry():
    def __init__(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.obj = obj
        self.mesh = mesh

class LocationEntry():
    def __init__(self, obj: bpy.types.Object, name: str):
        self.obj: bpy.types.Object = obj
        self.name: str = name
        self.on_enter: str = obj['on_enter'] if 'on_enter' in obj else ''
        self.room_index = 0

class LoadingZone():
    def __init__(self, obj: bpy.types.Object, target: str):
        self.obj: bpy.types.Object = obj
        self.target: str = target

    def bounding_box(self, scene_rotation: mathutils.Matrix) -> tuple[mathutils.Vector, mathutils.Vector]:
        final_transform = scene_rotation @ self.obj.matrix_world
        transformed = [final_transform @ mathutils.Vector(vtx) for vtx in self.obj.bound_box]

        bb_min = transformed[0]
        bb_max = transformed[0]

        for vtx in transformed:
            bb_min = mathutils.Vector((
                min(bb_min.x, vtx.x),
                min(bb_min.y, vtx.y),
                min(bb_min.z, vtx.z)
            ))
            bb_max = mathutils.Vector((
                max(bb_max.x, vtx.x),
                max(bb_max.y, vtx.y),
                max(bb_max.z, vtx.z)
            ))

        return bb_min, bb_max

class Scene():
    def __init__(self):
        self.static:list[StaticEntry] = []
        self.objects: list[ObjectEntry] = []
        self.locations: list[LocationEntry] = []
        self.loading_zones: list[LoadingZone] = []
        self.scene_mesh_collider = entities.mesh_collider.MeshCollider()

def process_linked_object(obj: bpy.types.Object, mesh: bpy.types.Mesh, definitions: dict[str, parse.struct_parse.StructureInfo]):
    type = None

    if 'type' in mesh:
        type = mesh['type']

    if 'type' in obj:
        type = obj['type']

    if not 'type' in mesh:
        return
    
    def_type_name = f"{type}_definition" 

    if not def_type_name in definitions:
        raise Exception(f"could not find def type {def_type_name}")
    
    print(f"found object {obj.name} of type {def_type_name}")

    return ObjectEntry(obj, type, definitions[def_type_name])

def write_static(scene: Scene, base_transform: mathutils.Matrix, room_collection: entities.room.room_collection, file):
    settings = entities.export_settings.ExportSettings()

    for entry in scene.static:
        room_collection.get_obj_room_index(entry.obj)

    mesh_list_for_rooms = []

    for i in range(len(room_collection.rooms)):
        mesh_list_for_rooms.append(entities.mesh.mesh_list(base_transform))

    for entry in scene.static:
        mesh_list_for_rooms[room_collection.get_obj_room_index(entry.obj)].append(entry.obj)

    meshes_for_rooms = list(map(lambda x: x.determine_mesh_data(), mesh_list_for_rooms))

    for i in range(len(meshes_for_rooms)):
        meshes_for_rooms[i] = list(map(lambda x: entities.mesh_optimizer.remove_duplicates(x), meshes_for_rooms[i]))

    meshes = []

    for room_meshes in meshes_for_rooms:
        for mesh in room_meshes:
            meshes.append(mesh)

    file.write(len(meshes).to_bytes(2, 'big'))

    for mesh in meshes:
        # this signals the mesh should be embedded
        file.write(b'\0')

        settings.default_material_name = entities.material_extract.material_romname(mesh.mat)
        settings.default_material = entities.material_extract.load_material_with_name(mesh.mat)

        entities.tiny3d_mesh_writer.write_mesh([mesh], None, [], settings, file)

    room_count = len(meshes_for_rooms)
    file.write(room_count.to_bytes(2, 'big'))
    index_start = 0

    for room_meshes in meshes_for_rooms:
        file.write(struct.pack('>HH', index_start, index_start + len(room_meshes)))
        index_start += len(room_meshes)

def find_static_blacklist():
    result = set()

    for collection in bpy.data.collections:
        if collection.name.startswith('lod_'):
            result = result.union(collection.all_objects)

    return result

def check_for_overworld(base_transform: mathutils.Matrix, overworld_filename: str, definitions, enums, variable_context):
    settings = entities.export_settings.ExportSettings()

    if not ('lod_1' in  bpy.data.collections):
        return False
    
    collection: bpy.types.Collection = bpy.data.collections["lod_1"]

    mesh_list = entities.mesh.mesh_list(base_transform)
    detail_list: list[entities.overworld.OverworldDetail] = []
    entity_list: list[entities.entities.ObjectEntry] = []

    collider = entities.mesh_collider.MeshCollider()

    for obj in collection.all_objects:
        final_transform = base_transform @ obj.matrix_world

        if obj.type != "MESH":
            continue

        if 'type' in obj or 'type' in obj.data:
            entity_list.append(process_linked_object(obj, obj.data, definitions))
            continue

        mesh: bpy.types.Mesh = obj.data

        if mesh.library:
            detail_list.append(entities.overworld.OverworldDetail(obj))
            continue

        if len(mesh.materials) > 0:
            mesh_list.append(obj)

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            collider.append(mesh, final_transform)

    lod_0_collection: bpy.types.Collection | None = bpy.data.collections["lod_0"] if "lod_0" in bpy.data.collections else None
    lod_0_objects = []

    if lod_0_collection:
        lod_0_objects = list(lod_0_collection.all_objects)

    subdivisions = collection['subdivisions'] if 'subdivisions' in collection else 8

    entities.overworld.generate_overworld(
        overworld_filename, 
        mesh_list, 
        lod_0_objects, 
        collider, 
        detail_list, 
        entity_list,
        subdivisions, 
        settings, 
        base_transform,
        enums,
        variable_context
    )

    return True
    
def process_scene():
    input_filename = sys.argv[1]
    output_filename = sys.argv[-2]
    overworld_filename = sys.argv[-1]

    scene = Scene()

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
    definitions = {}
    enums = {}

    with open('src/scene/scene_definition.h', 'r') as file:
        file_content = file.read()
        definitions = parse.struct_parse.find_structs(file_content)
        enums = parse.struct_parse.find_enums(file_content)

    globals = cutscene.variable_layout.VariableLayout()

    with open('build/assets/scripts/globals.json') as file:
        globals.deserialize(file)

    variable_context = cutscene.variable_layout.VariableContext(globals, cutscene.variable_layout.VariableLayout())

    context = parse.struct_serialize.SerializeContext(enums)

    object_blacklist = find_static_blacklist()

    has_overworld = check_for_overworld(base_transform, overworld_filename, definitions, enums, variable_context)

    for obj in bpy.data.objects:
        if obj.name.startswith('fast64_f3d_material_library_'):
            continue

        if obj in object_blacklist:
            continue

        if 'loading_zone' in obj:
            scene.loading_zones.append(LoadingZone(obj, obj['loading_zone']))
            continue

        if entities.entry_point.is_entry_point(obj):
            scene.locations.append(LocationEntry(obj, entities.entry_point.get_entry_point(obj)))
            continue

        if 'type' in obj or 'type' in obj.data:
            scene.objects.append(process_linked_object(obj, obj.data, definitions))
            continue

        if obj.type != "MESH":
            continue

        final_transform = base_transform @ obj.matrix_world

        mesh: bpy.types.Mesh = obj.data

        if len(mesh.materials) > 0 and not obj.name.startswith('collision'):
            scene.static.append(StaticEntry(obj, mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH' or obj.name.startswith('collision'):
            scene.scene_mesh_collider.append(mesh, final_transform)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

        file.write(len(scene.locations).to_bytes(1, 'big'))

        room_collection = entities.room.room_collection()

        for location in scene.locations:
            location_bytes = location.name.encode()
            file.write(len(location_bytes).to_bytes(1, 'big'))
            file.write(location_bytes)

            on_enter_bytes = location.on_enter.encode()
            file.write(len(on_enter_bytes).to_bytes(1, 'big'))
            file.write(on_enter_bytes)

            parse.struct_serialize.write_vector3_position(file, location.obj)
            parse.struct_serialize.write_vector2_rotation(file, location.obj)

            file.write(struct.pack('>H', room_collection.get_obj_room_index(location.obj)))

        write_static(scene, base_transform, room_collection, file)

        scene.scene_mesh_collider.write_out(file)

        grouped: dict[str, list[ObjectEntry]] = {}

        for object in scene.objects:
            key = object.name

            parse.struct_serialize.layout_strings(object.obj, object.def_type, context, None)

            if key in grouped:
                grouped[key].append(object)
            else:
                grouped[key] = [object]

        for loading_zone in scene.loading_zones:
            context.get_string_offset(loading_zone.target)

        context.write_strings(file)

        grouped_list = sorted(list(grouped.items()), key=lambda x: x[0])

        file.write(len(grouped_list).to_bytes(2, 'big'))

        for item in grouped_list:
            def_type = enums['enum entity_type_id'].str_to_int('ENTITY_TYPE_' + item[0])
            file.write(def_type.to_bytes(2, 'big'))

            file.write(len(item[1]).to_bytes(2, 'big'))
            struct_size = parse.struct_serialize.obj_gather_types(item[1][0].def_type, context)
            file.write(struct_size.to_bytes(2, 'big'))
            
            for entry in item[1]:
                entry.write_definition(context, file)

            for entry in item[1]:
                entry.write_condition(variable_context, file)

        file.write(struct.pack(">H", len(scene.loading_zones)))

        for loading_zone in scene.loading_zones:
            bb_min, bb_max = loading_zone.bounding_box(base_transform)
            file.write(struct.pack(">fff", bb_min.x, bb_min.y, bb_min.z))
            file.write(struct.pack(">fff", bb_max.x, bb_max.y, bb_max.z))
            file.write(struct.pack(">I", context.get_string_offset(loading_zone.target)))

        if has_overworld:
            overworld_romname = overworld_filename.replace('filesystem/', 'rom:/')
            file.write(struct.pack(">B", len(overworld_romname)))
            file.write(overworld_romname.encode())
        else:
            file.write(b'\0')

        entities.camera_animation.export_camera_animations(output_filename.replace('.scene', '.sanim'), file)
            

process_scene()