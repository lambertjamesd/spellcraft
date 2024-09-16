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
import entities.mesh_optimizer
import entities.material_extract
import entities.animation
import entities.armature

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

def process_scene():
    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    arm: entities.armature.ArmatureData | None = None

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if not obj.name in bpy.context.view_layer.objects:
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
    attatchments: list[entities.armature.BoneLinkage] = []

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if obj.data.library:
            if obj.parent and obj.parent_bone and arm:
                attatchments.append(arm.find_bone_linkage(obj))
            
            continue

        mesh_list.append(obj)

    settings = entities.export_settings.ExportSettings()

    meshes = mesh_list.determine_mesh_data(arm)

    meshes = list(map(lambda x: (x[0], entities.mesh_optimizer.remove_duplicates(x[1])), meshes))

    if len(meshes) == 1 and meshes[0][0].startswith('materials/'):
        settings.default_material_name = meshes[0][0]
        settings.default_material = entities.material_extract.load_material_with_name(meshes[0][0], meshes[0][1].mat)

    with open(sys.argv[-1], 'wb') as file:
        entities.tiny3d_mesh_writer.write_mesh(meshes, arm, attatchments, settings, file)

    entities.animation.export_animations(replace_extension(sys.argv[-1], '.anim'), arm, settings)
    
process_scene()