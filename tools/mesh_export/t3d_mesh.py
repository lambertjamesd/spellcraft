import bpy
import mathutils
import bmesh
import sys
import math
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.tiny3d_mesh_writer
import entities.export_settings

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

def process_scene():
    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    arm: entities.armature.ArmatureData | None = None

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        bpy.context.view_layer.objects.active = obj

        for modifier in obj.modifiers:
            if modifier.type != 'MIRROR':
                continue
            bpy.ops.object.modifier_apply(modifier=modifier.name, single_user = True)


    for obj in bpy.data.objects:
        if obj.type != 'ARMATURE':
            continue

        if not arm is None:
            raise Exception('Only 1 armature allowed')

        arm = entities.armature.ArmatureData(obj, base_transform)

    if arm:
        for obj in bpy.data.objects:
            arm.check_connected_mesh_object(obj)

        arm.build_bone_data()

    for mesh in bpy.data.meshes:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()

    mesh_list = entities.mesh.mesh_list(base_transform)

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        mesh_list.append(obj)

    settings = entities.export_settings.ExportSettings()

    with open(sys.argv[-1], 'wb') as file:
        meshes = mesh_list.determine_mesh_data(arm)
        entities.tiny3d_mesh_writer.write_mesh(meshes, settings, file)

process_scene()