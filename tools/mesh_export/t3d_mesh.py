import bpy
import mathutils
import bmesh
import sys
import math
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.mesh
import mesh_export.entities.tiny3d_mesh_writer
import mesh_export.entities.export_settings
import mesh_export.entities.mesh_optimizer
import mesh_export.entities.material_extract
import mesh_export.entities.animation
import mesh_export.entities.armature

from mesh_export.deps import generate_deps

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

SKIPPED_MODIFIERS = {
    "ARMATURE",
}

def process_scene():
    output_filename = sys.argv[-1]

    generate_deps.generate_deps(output_filename, os.path.relpath(__file__))

    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    arm: mesh_export.entities.armature.ArmatureData | None = None

    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue

        if not obj.name in bpy.context.view_layer.objects:
            continue

        bpy.context.view_layer.objects.active = obj

        for modifier in obj.modifiers:
            if modifier.type in SKIPPED_MODIFIERS or not modifier.show_render:
                print("skipping modifier " + modifier.type)
                continue
            bpy.ops.object.modifier_apply(modifier=modifier.name, single_user = True)


    for obj in bpy.data.objects:
        if obj.type != 'ARMATURE' or obj.hide_render:
            continue

        if not arm is None:
            raise Exception('Only 1 armature allowed')

        arm = mesh_export.entities.armature.ArmatureData(obj, base_transform)

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

    mesh_list = mesh_export.entities.mesh.mesh_list(base_transform)
    attachments: list[mesh_export.entities.armature.BoneLinkage] = []

    use_scene = None

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if obj.data.library:
            if obj.parent and obj.parent_bone and arm:
                attachments.append(arm.find_bone_linkage(obj))
            
            continue

        mesh_list.append(obj)
        use_scene = obj.users_scene[0]

    settings = mesh_export.entities.export_settings.ExportSettings()
    settings.fixed_point_scale = 128

    meshes = mesh_list.determine_mesh_data(arm)

    meshes = list(map(lambda x: mesh_export.entities.mesh_optimizer.remove_duplicates(x), meshes))

    if not use_scene:
        raise Exception('could not find a scene')

    if 'default_material' in use_scene and use_scene['default_material']:
        default_material = use_scene['default_material']
        settings.default_material_name = mesh_export.entities.material_extract.material_romname(default_material)
        settings.default_material = mesh_export.entities.material_extract.load_material_with_name(default_material)
    elif len(meshes) == 1 and mesh_export.entities.material_extract.material_can_extract(meshes[0].mat):
        settings.default_material_name = mesh_export.entities.material_extract.material_romname(meshes[0].mat)
        settings.default_material = mesh_export.entities.material_extract.load_material_with_name(meshes[0].mat)

    if 'light_source' in use_scene:
        settings.light_source = use_scene['light_source']

    with open(output_filename, 'wb') as file:
        mesh_export.entities.tiny3d_mesh_writer.write_mesh(meshes, arm, attachments, settings, file)

    mesh_export.entities.animation.export_animations(replace_extension(output_filename, '.anim'), arm, settings)
    
process_scene()