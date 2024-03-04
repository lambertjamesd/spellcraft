import bpy
import sys
import os
import mathutils
import math

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.mesh_collider
import parse.struct_parse

class StaticEntry():
    def __init__(self, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.mesh = mesh
        self.transform = transform

class ObjectEntry():
    def __init__(self, obj: bpy.types.Object, def_type: parse.struct_parse.StructureInfo):
        self.obj: bpy.types.Object = obj
        self.def_type: parse.struct_parse.StructureInfo = def_type

class World():
    def __init__(self):
        self.static = []
        self.objects = []
        self.world_mesh_collider = entities.mesh_collider.MeshCollider()

def process_linked_object(world: World, obj: bpy.types.Object, mesh: bpy.types.Mesh, definitions: dict[str, parse.struct_parse.StructureInfo]):
    if not 'type' in mesh:
        return
    
    def_type_name = mesh['type']

    if not def_type_name in definitions:
        raise Exception(f"could not find def type {def_type_name}")
    
    print(f"found object {obj.name} of type {def_type_name}")
    
    world.objects.append(ObjectEntry(obj, definitions[def_type_name]))
    
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
            world.static.append(StaticEntry(mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            world.world_mesh_collider.append(mesh, final_transform)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

        file.write(len(world.static).to_bytes(2, 'big'))

        for entry in world.static:
            # this signals the mesh should be embedded
            file.write((0).to_bytes(1, 'big'))

            mesh_list = entities.mesh.mesh_list()
            mesh_list.append(entry.mesh, entry.transform)
            mesh_list.write_mesh(file)

        world.world_mesh_collider.write_out(file)

process_scene()