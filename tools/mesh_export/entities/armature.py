import bpy
import sys
import os
import mathutils

sys.path.append(os.path.dirname(__file__))

class ArmatureBone:
    def __init__(self, index: int, bone: bpy.types.Bone, armature_transform: mathutils.Matrix):
        self.index: int = index
        self.bone: bpy.types.Bone = bone
        self.matrix_world: mathutils.Matrix = armature_transform @ bone.matrix_local
        self.matrix_world_inv: mathutils.Matrix = self.matrix_world.inverted()
        self.matrix_normal_inv = self.matrix_world_inv.to_3x3().inverted().transposed()

class ArmatureData:
    def __init__(self, obj: bpy.types.Object, base_transform: mathutils.Matrix):
        self.transfrom: mathutils.Matrix = base_transform @ obj.matrix_world
        self.armature: bpy.types.Armature = obj.data
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
        for bone in self.armature.bones:
            if bone.name not in self.used_bones:
                continue

            self._filtered_bones[bone.name] = ArmatureBone(
                len(self._filtered_bones),
                bone,
                self.transfrom
            )

    def find_bone_data(self, bone_name: str):
        return self._filtered_bones[bone_name]
    
    def find_parent_bone(self, bone_name: str):
        curr = self.find_bone_data(bone_name)

        result = curr.bone.parent

        while result and not result.name in self.used_bones:
            result = result.parent

        if result is None:
            return None
        
        return self._filtered_bones[result.name]
    
    def get_filtered_bones(self) -> list[ArmatureBone]:
        return sorted(list(self._filtered_bones.values()), key = lambda x: x.index)