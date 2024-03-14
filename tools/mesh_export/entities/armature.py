import bpy
import sys
import os
import mathutils

sys.path.append(os.path.dirname(__file__))

class ArmatureBone:
    def __init__(self, index: int, bone: bpy.types.Bone, edit_bone: bpy.types.EditBone, pose_bone: bpy.types.PoseBone, armature_transform: mathutils.Matrix):
        self.index: int = index
        self.name: str = bone.name
        self.parent_names: list[str] = []

        curr = bone.parent

        while curr:
            self.parent_names.append(curr.name)
            curr = curr.parent

        self.matrix_world: mathutils.Matrix = armature_transform @ edit_bone.matrix
        self.matrix_world_inv: mathutils.Matrix = self.matrix_world.inverted()
        self.matrix_normal_inv = self.matrix_world_inv.to_3x3().inverted().transposed()

        self.pose_matrix = pose_bone.matrix
        self.pose_matrix_inv = self.pose_matrix.inverted()

class ArmatureData:
    def __init__(self, obj: bpy.types.Object, base_transform: mathutils.Matrix):
        self.relative_vertex_transform: mathutils.Matrix = base_transform @ obj.matrix_world
        self.relative_bone_transform: mathutils.Matrix = base_transform @ obj.matrix_world.inverted()
        self.armature: bpy.types.Armature = obj.data
        self.obj: bpy.types.Object = obj
        self.used_bones: set[str] = set()

        self._filtered_bones: dict[str, ArmatureBone] = {}

    def check_connected_mesh_object(self, obj: bpy.types.Object):
        if obj.type != "MESH":
            return

        if obj.parent is None or obj.parent.data != self.armature:
            return

        self.mark_bone_used(obj.parent_bone)

        mesh: bpy.types.Mesh = obj.data

        used_group_indices = set()

        for vertex in mesh.vertices:
            for group in vertex.groups:
                if group.weight > 0:
                    used_group_indices.add(group.group)
                
        for group_index in used_group_indices:
            self.mark_bone_used(obj.vertex_groups[group_index].name)


    def mark_bone_used(self, name: str | None):
        if name is None or len(name) == 0:
            return
        
        bone: bpy.types.Bone = self.armature.bones.get(name)

        if bone is None:
            return
        
        # check if already added
        if name is self.used_bones:
            return

        self.used_bones.add(name)

    def build_bone_data(self):
        bpy.ops.object.select_all(action = 'DESELECT')
        self.obj.select_set(True)
        bpy.context.view_layer.objects.active = self.obj
        bpy.ops.object.editmode_toggle()

        for idx, bone in enumerate(self.armature.bones):
            if bone.name not in self.used_bones:
                continue

            self._filtered_bones[bone.name] = ArmatureBone(
                len(self._filtered_bones),
                bone,
                self.armature.edit_bones[idx],
                self.obj.pose.bones[idx],
                self.relative_vertex_transform
            )
        
        bpy.ops.object.editmode_toggle()

    def find_bone_data(self, bone_name: str):
        return self._filtered_bones[bone_name]
    
    def find_parent_bone(self, bone_name: str):
        curr = self.find_bone_data(bone_name)

        for parent_name in curr.parent_names:
            if parent_name in self.used_bones:
                return self._filtered_bones[parent_name]
            
        return None
    
    def get_filtered_bones(self) -> list[ArmatureBone]:
        return sorted(list(self._filtered_bones.values()), key = lambda x: x.index)