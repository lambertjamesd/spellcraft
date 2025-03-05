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
import entities.overworld
import parse.struct_parse
import parse.struct_serialize
import cutscene.expresion_generator
import cutscene.parser
import cutscene.variable_layout

class StaticEntry():
    def __init__(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.obj = obj
        self.mesh = mesh

class ObjectEntry():
    def __init__(self, obj: bpy.types.Object, name: str, def_type: parse.struct_parse.StructureInfo):
        self.obj: bpy.types.Object = obj
        self.name: str = name
        self.def_type: parse.struct_parse.StructureInfo = def_type

class LocationEntry():
    def __init__(self, obj: bpy.types.Object, name: str):
        self.obj: bpy.types.Object = obj
        self.name: str = name
        self.on_enter: str = obj['on_enter'] if 'on_enter' in obj else ''

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

def process_linked_object(scene: Scene, obj: bpy.types.Object, mesh: bpy.types.Mesh, definitions: dict[str, parse.struct_parse.StructureInfo]):
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
    
    scene.objects.append(ObjectEntry(obj, type, definitions[def_type_name]))

def write_static(scene: Scene, base_transform: mathutils.Matrix, file):
    settings = entities.export_settings.ExportSettings()
    mesh_list = entities.mesh.mesh_list(base_transform)

    for entry in scene.static:
        mesh_list.append(entry.obj)

    meshes = mesh_list.determine_mesh_data()
    meshes = list(map(lambda x: (x[0], entities.mesh_optimizer.remove_duplicates(x[1])), meshes))

    file.write(len(meshes).to_bytes(2, 'big'))

    for mesh in meshes:
        # this signals the mesh should be embedded
        file.write(b'\0')

        settings.default_material_name = entities.material_extract.material_romname(mesh[1].mat)
        settings.default_material = entities.material_extract.load_material_with_name(mesh[0], mesh[1].mat)

        entities.tiny3d_mesh_writer.write_mesh([mesh], None, [], settings, file)

def find_static_blacklist():
    result = set()

    for collection in bpy.data.collections:
        if collection.name.startswith('lod_'):
            result = result.union(collection.all_objects)

    return result

def check_for_overworld(base_transform: mathutils.Matrix, overworld_filename: str):
    settings = entities.export_settings.ExportSettings()

    if not ('lod_1' in  bpy.data.collections):
        return False
    
    collection: bpy.types.Collection = bpy.data.collections["lod_1"]

    mesh_list = entities.mesh.mesh_list(base_transform)

    for obj in collection.all_objects:
        mesh_list.append(obj)

    lod_0_mesh = entities.mesh.mesh_list(base_transform)

    lod_0_collection: bpy.types.Collection | None = bpy.data.collections["lod_0"] if "lod_0" in bpy.data.collections else None

    if lod_0_collection:
        for obj in sorted(lod_0_collection.all_objects, key=lambda x: x.name):
            lod_0_mesh.append(obj)

    subdivisions = collection['subdivisions'] if 'subdivisions' in collection else 8

    entities.overworld.generate_overworld(overworld_filename, mesh_list, lod_0_mesh, subdivisions, settings)

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

    has_overworld = check_for_overworld(base_transform, overworld_filename)

    for obj in bpy.data.objects:
        if 'loading_zone' in obj:
            scene.loading_zones.append(LoadingZone(obj, obj['loading_zone']))
            continue

        if entities.entry_point.is_entry_point(obj):
            scene.locations.append(LocationEntry(obj, entities.entry_point.get_entry_point(obj)))
            continue

        if 'type' in obj or 'type' in obj.data:
            process_linked_object(scene, obj, obj.data, definitions)
            continue

        if obj.type != "MESH":
            continue

        if obj.name.startswith('fast64_f3d_material_library_'):
            continue

        if obj in object_blacklist:
            continue

        final_transform = base_transform @ obj.matrix_world

        mesh: bpy.types.Mesh = obj.data

        if len(mesh.materials) > 0:
            scene.static.append(StaticEntry(obj, mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            scene.scene_mesh_collider.append(mesh, final_transform)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

        file.write(len(scene.locations).to_bytes(1, 'big'))

        for location in scene.locations:
            location_bytes = location.name.encode()
            file.write(len(location_bytes).to_bytes(1, 'big'))
            file.write(location_bytes)

            on_enter_bytes = location.on_enter.encode()
            file.write(len(on_enter_bytes).to_bytes(1, 'big'))
            file.write(on_enter_bytes)

            parse.struct_serialize.write_vector3_position(file, location.obj)
            parse.struct_serialize.write_vector2_rotation(file, location.obj)

        write_static(scene, base_transform, file)

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
            def_name = item[0]

            file.write(len(def_name).to_bytes(1, 'big'))
            file.write(def_name.encode())

            type_locations: list[parse.struct_serialize.TypeLocation] = []

            file.write(len(item[1]).to_bytes(2, 'big'))
            struct_size = parse.struct_serialize.obj_gather_types(item[1][0].def_type, type_locations, context)
            file.write(struct_size.to_bytes(2, 'big'))

            file.write(struct.pack(">B", len(type_locations)))

            for type_location in type_locations:
                file.write(struct.pack(">BB", type_location.type_id, type_location.offset))
            
            for entry in item[1]:
                parse.struct_serialize.write_obj(file, entry.obj, entry.def_type, context)

            for entry in item[1]:
                condition_text = entry.obj['condition'] if 'condition' in entry.obj else 'true'
                condition = cutscene.parser.parse_expression(condition_text, entry.obj.name + ".condition")
                script = cutscene.expresion_generator.generate_script(condition, variable_context, 'int')
                script.serialize(file)

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
            

process_scene()