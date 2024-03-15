import bpy
import sys
import os
import mathutils
import math

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

class World():
    def __init__(self):
        self.static:list[StaticEntry] = []
        self.objects: list[ObjectEntry] = []
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

    with open('src/scene/world_definition.h', 'r') as file:
        definitions = parse.struct_parse.find_structs(file.read())

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if obj.name.startswith('@'):
            # TODO process non static objects
            continue

        final_transform = base_transform @ obj.matrix_world

        mesh: bpy.types.Mesh = obj.data

        mesh_source = None

        if mesh.library:
            process_linked_object(world, obj, mesh, definitions)
        elif len(mesh.materials) > 0:
            world.static.append(StaticEntry(obj, mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            world.world_mesh_collider.append(mesh, final_transform)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

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

            if key in grouped:
                grouped[key].append(object)
            else:
                grouped[key] = [object]

        grouped_list = sorted(list(grouped.items()), key=lambda x: x[0])

        file.write(len(grouped_list).to_bytes(2, 'big'))

        for item in grouped_list:
            def_name = item[0]

            file.write(len(def_name).to_bytes(1, 'big'))
            file.write(def_name.encode())

            file.write(len(item[1]).to_bytes(2, 'big'))
            file.write(parse.struct_serialize.obj_size(item[1][0].def_type).to_bytes(2, 'big'))
            
            for entry in item[1]:
                parse.struct_serialize.write_obj(file, entry.obj, entry.def_type)
            

process_scene()