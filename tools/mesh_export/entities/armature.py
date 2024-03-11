import bpy
import sys
import os

sys.path.append(os.path.dirname(__file__))

class ArmatureData:
    def __init__(self, armature: bpy.types.Armature):
        self.armature: bpy.types.Armature = armature
        self.used_bones: set[str] = set()

    def check_connected_mesh_object(self, obj: bpy.types.Object):
        if obj.type != "MESH":
            return

        if obj.parent is None or obj.parent.data != self.armature:
            return

        self.mark_bone_used(obj.parent_bone)

        for group in obj.vertex_groups:
            self.mark_bone_used(group.name)


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

        if bone.parent:
            self.mark_bone_used(bone.parent.name)