import bpy
import mathutils
import bmesh
import sys
import math
import os.path
import subprocess

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
import mesh_export.entities.scene_process

from mesh_export.deps import generate_deps

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

PART_PREFIX = 'part#'

def get_part(obj: bpy.types.Object) -> str:
    # prefer collections that directly contain the object
    for collection in obj.users_collection:
        if collection.name.startswith(PART_PREFIX):
            return collection.name[len(PART_PREFIX):]

    # fall back to any collection that contains the object through nesting
    for collection in bpy.data.collections:
        if collection.name.startswith(PART_PREFIX) and obj.name in collection.all_objects:
            return collection.name[len(PART_PREFIX):]

    return 'default'

class part_data():
    def __init__(self, base_transform: mathutils.Matrix):
        self.mesh_list: mesh_export.entities.mesh.mesh_list = mesh_export.entities.mesh.mesh_list(base_transform)
        self.attachments: list[mesh_export.entities.armature.BoneLinkage] = []
        self.use_scene: bpy.types.Scene | None = None
        self.arm: mesh_export.entities.armature.ArmatureData | None = None

def process_part(name: str, data: part_data, mk_asset: str, build_dir: str, output_dir: str, base_name: str):
    settings = mesh_export.entities.export_settings.ExportSettings()
    settings.fixed_point_scale = 128

    arm = data.arm
    mesh_list = data.mesh_list
    use_scene = data.use_scene
    attachments = data.attachments

    meshes = mesh_list.determine_mesh_data(arm)

    meshes = list(map(lambda x: mesh_export.entities.mesh_optimizer.remove_duplicates(x), meshes))

    if not use_scene:
        raise Exception('could not find a scene')

    if hasattr(use_scene, 'default_material') and use_scene.default_material:
        default_material = use_scene.default_material
        settings.default_material_name = mesh_export.entities.material_extract.material_romname(default_material)
        settings.default_material = mesh_export.entities.material_extract.load_material_with_name(default_material)
        print('using default material', default_material.name)
    elif len(meshes) == 1 and mesh_export.entities.material_extract.material_can_extract(meshes[0].mat):
        settings.default_material_name = mesh_export.entities.material_extract.material_romname(meshes[0].mat)
        settings.default_material = mesh_export.entities.material_extract.load_material_with_name(meshes[0].mat)
        print('single material, using as default')
    else:
        priority = 100

        for mesh in meshes:
            mesh_priority = mesh_export.entities.material_extract.load_material_with_name(mesh.mat).priority

            if mesh_priority is not None and mesh_priority < priority:
                priority = mesh_priority

        if priority != 100 and priority != 1:
            print(f"using mesh priority {priority}")
            settings.default_material.priority = priority
            settings.default_material_name = ""

        print('no default material')

    if 'light_source' in use_scene:
        settings.light_source = use_scene['light_source']

    part_name = base_name if name == 'default' else f"{base_name}_{name}"

    # uncompressed intermediate mesh goes in the build directory
    intermediate_filename = os.path.join(build_dir, f"{part_name}.tmesh")

    with open(intermediate_filename, 'wb') as file:
        mesh_export.entities.tiny3d_mesh_writer.write_mesh(meshes, arm, attachments, settings, file)

    # compress the mesh into the final output directory
    subprocess.run([mk_asset, '-o', output_dir, '-w', '256', intermediate_filename], check=True)

    # animations are written uncompressed directly to the final output
    mesh_export.entities.animation.export_animations(os.path.join(output_dir, f"{part_name}.anim"), arm, settings)


def process_scene():
    mk_asset, build_dir, output_dir = sys.argv[-3:]

    base_name = os.path.splitext(os.path.basename(bpy.data.filepath))[0]

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)

    generate_deps.generate_deps(os.path.join(output_dir, f"{base_name}.tmesh"), os.path.relpath(__file__))

    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    parts: dict[str, part_data] = {}
    
    def get_part_data(name: str) -> part_data:
        if name in parts:
            return parts[name]

        result = part_data(base_transform)
        parts[name] = result
        return result

    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue

        if not obj.name in bpy.context.view_layer.objects:
            continue

        mesh_export.entities.scene_process.apply_modifiers(obj)


    for obj in bpy.data.objects:
        if obj.type != 'ARMATURE' or obj.hide_render:
            continue

        part = get_part_data(get_part(obj))

        if not part.arm is None:
            raise Exception('Only 1 armature allowed')

        part.arm = mesh_export.entities.armature.ArmatureData(obj, base_transform)

    for obj in bpy.data.objects:
        part = get_part(obj)

        part_arm = get_part_data(part).arm

        if part_arm:
            part_arm.check_connected_mesh_object(obj)

    for data in parts.values():
        if data.arm:
            data.arm.build_bone_data()


    for mesh in bpy.data.meshes:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()

    for obj in bpy.data.objects:
        if obj.type != "MESH" or not obj.data:
            continue

        part = get_part_data(get_part(obj))

        if obj.data.library:
            if obj.parent and obj.parent_bone and part.arm:
                part.attachments.append(part.arm.find_bone_linkage(obj))
            
            continue

        part.mesh_list.append(obj)
        part.use_scene = obj.users_scene[0]

    for part, data in parts.items():
        process_part(part, data, mk_asset, build_dir, output_dir, base_name)
    
process_scene()