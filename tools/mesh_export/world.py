import bpy
import sys
import os
import mathutils
import math
import struct

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.mesh_collider
import parse.struct_parse
import parse.struct_serialize

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

class World():
    def __init__(self):
        self.static:list[StaticEntry] = []
        self.objects: list[ObjectEntry] = []
        self.locations: list[LocationEntry] = []
        self.world_mesh_collider = entities.mesh_collider.MeshCollider()

def process_linked_object(world: World, obj: bpy.types.Object, mesh: bpy.types.Mesh, definitions: dict[str, parse.struct_parse.StructureInfo]):
    if not 'type' in mesh:
        return
    
    def_type_name = f"{mesh['type']}_definition" 

    if not def_type_name in definitions:
        raise Exception(f"could not find def type {def_type_name}")
    
    print(f"found object {obj.name} of type {def_type_name}")
    
    world.objects.append(ObjectEntry(obj, mesh['type'], definitions[def_type_name]))
    
def process_scene():
    input_filename = sys.argv[1]
    output_filename = sys.argv[-1]

    world = World()

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
    definitions = {}
    enums = {}

    with open('src/scene/world_definition.h', 'r') as file:
        file_content = file.read()
        definitions = parse.struct_parse.find_structs(file_content)
        enums = parse.struct_parse.find_enums(file_content)

    context = parse.struct_serialize.SerializeContext(enums)

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            if 'entry_point' in obj:
                world.locations.append(LocationEntry(obj, obj['entry_point']))
            continue

        if obj.name.startswith('@'):
            # TODO process non static objects
            continue

        final_transform = base_transform @ obj.matrix_world

        mesh: bpy.types.Mesh = obj.data

        mesh_source = None

        if 'type' in mesh:
            process_linked_object(world, obj, mesh, definitions)
        elif len(mesh.materials) > 0:
            world.static.append(StaticEntry(obj, mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            world.world_mesh_collider.append(mesh, final_transform)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

        file.write(len(world.locations).to_bytes(1, 'big'))

        for location in world.locations:
            location_bytes = location.name.encode()
            file.write(len(location_bytes).to_bytes(1, 'big'))
            file.write(location_bytes)

            parse.struct_serialize.write_vector3_position(file, location.obj)
            parse.struct_serialize.write_vector2_rotation(file, location.obj)

        file.write(len(world.static).to_bytes(2, 'big'))

        for entry in world.static:
            # this signals the mesh should be embedded
            file.write((0).to_bytes(1, 'big'))

            mesh_list = entities.mesh.mesh_list(base_transform)
            mesh_list.append(entry.obj)
            mesh_list.write_mesh(file)

        world.world_mesh_collider.write_out(file)

        grouped: dict[str, list[ObjectEntry]] = {}

        for object in world.objects:
            key = object.name

            parse.struct_serialize.layout_strings(object.obj, object.def_type, context, None)

            if key in grouped:
                grouped[key].append(object)
            else:
                grouped[key] = [object]

        context.write_strings(file)

        grouped_list = sorted(list(grouped.items()), key=lambda x: x[0])

        file.write(len(grouped_list).to_bytes(2, 'big'))

        for item in grouped_list:
            def_name = item[0]

            file.write(len(def_name).to_bytes(1, 'big'))
            file.write(def_name.encode())

            type_locations: list[parse.struct_serialize.TypeLocation] = []

            file.write(len(item[1]).to_bytes(2, 'big'))
            struct_size, struct_alignment = parse.struct_serialize.obj_gather_types(item[1][0].def_type, type_locations, context)
            file.write(struct_size.to_bytes(2, 'big'))

            file.write(struct.pack(">B", len(type_locations)))

            for type_location in type_locations:
                file.write(struct.pack(">BB", type_location.type_id, type_location.offset))
            
            for entry in item[1]:
                parse.struct_serialize.write_obj(file, entry.obj, entry.def_type, context)

            

process_scene()