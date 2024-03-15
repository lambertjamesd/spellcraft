import bpy
import mathutils
import bmesh
import sys
import math
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.armature

def process_scene():
    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    armature: entities.armature.ArmatureData | None = None

    for obj in bpy.data.objects:
        if obj.type != 'ARMATURE':
            continue

        if not armature is None:
            raise Exception('Only 1 armature allowed')

        armature = entities.armature.ArmatureData(obj, base_transform)

    if armature:
        for obj in bpy.data.objects:
            armature.check_connected_mesh_object(obj)

        armature.build_bone_data()

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
        mesh_list.write_mesh(file, armature = armature)

process_scene()