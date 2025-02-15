import bpy
import mathutils
import bmesh
import sys
import math
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.armature
import entities.export_settings

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

def export_animations(arm: entities.armature.ArmatureData | None, settings: entities.export_settings.ExportSettings):
    if not arm:
        return
    
    animations = list(filter(lambda action: arm.is_action_compatible(action), bpy.data.actions))

    bones = arm.get_filtered_bones()
    attributes_for_anim: list[entities.armature.ArmatureAttributes] = []
    packed_animations: list[entities.armature.PackedAnimation] = []

    default_pose = arm.generate_pose_data(settings)

    start_action = arm.obj.animation_data.action if arm.obj.animation_data else None
    start_frame = bpy.context.scene.frame_current

    for anim in animations:
        arm.obj.animation_data.action = anim

        frame_start = int(anim.frame_range[0])
        frame_end = int(anim.frame_range[1])

        packed_animation = entities.armature.PackedAnimation()

        for frame in range(frame_start, frame_end):
            bpy.context.scene.frame_set(frame)
            packed_animation.add_frame(arm.generate_pose_data(settings), arm.generate_event_data())

        attributes = packed_animation.determine_needed_channels(default_pose)
        attributes_for_anim.append(attributes)
        packed_animations.append(packed_animation)

    if arm.obj.animation_data:
        arm.obj.animation_data.action = start_action
    bpy.context.scene.frame_set(start_frame)

    name_lengths = 0

    for anim in animations:
        name_lengths += len(anim.name.encode()) + 1

    with open(replace_extension(sys.argv[-1], '.anim'), 'wb') as file:
        file.write('ANIM'.encode())
        file.write(len(animations).to_bytes(2, 'big'))
        file.write(name_lengths.to_bytes(2, 'big'))
        file.write(len(bones).to_bytes(2, 'big'))

        # write headers
        for idx, packed_anim in enumerate(packed_animations):
            frame_count = packed_anim.frame_count()
            file.write(frame_count.to_bytes(2, 'big'))
            file.write(bpy.context.scene.render.fps.to_bytes(2, 'big'))
            file.write(attributes_for_anim[idx].determine_attribute_list_size().to_bytes(2, 'big'))
            file.write(attributes_for_anim[idx].determine_used_flags().to_bytes(2, 'big'))

        attr_size = 0
        # write used attributes
        for attributes in attributes_for_anim:
            for attr in attributes.bone_attributes:
                attr_bytes = attr.to_bytes()
                file.write(attr_bytes)
                attr_size += len(attr_bytes)

        for anim in animations:
            name_bytes = anim.name.encode()
            file.write(name_bytes)
            file.write(b'\0')

        for anim_idx, anim in enumerate(packed_animations):
            # align to 2 bytes
            current_size = file.tell()

            if current_size & 1:
                file.write(b'\0')
                
            anim.write_to_file(file, attributes_for_anim[anim_idx])


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

    with open(sys.argv[-1], 'wb') as file:
        mesh_list.write_mesh(file, armature = arm)

    export_animations(arm, entities.export_settings.ExportSettings())

process_scene()