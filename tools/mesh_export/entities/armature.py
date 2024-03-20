import bpy
import sys
import os
import mathutils
import struct
import math

sys.path.append(os.path.dirname(__file__))

class BoneAttributes():
    def __init__(self):
        self.has_pos = False
        self.has_rot = False
        self.has_scale = False

    def __str__(self) -> str:
        return f"{'pos ' if self.has_pos else ''}{'rot ' if self.has_rot else ''}{'scale' if self.has_scale else ''}"
    
    def to_bytes(self) -> bytes:
        result = 0

        if self.has_pos:
            result |= 128
        if self.has_rot:
            result |= 64
        if self.has_scale:
            result |= 32

        return result.to_bytes(1, 'big')
    
class PackedArmatureData():
    def __init__(self, loc: mathutils.Vector, rot: mathutils.Quaternion, scale: mathutils.Vector):
        packed_rotation = _pack_quaternion(rot)
        self._data: list[int] = [
            _pack_position(loc.x),
            _pack_position(loc.y),
            _pack_position(loc.z),
            packed_rotation[0],
            packed_rotation[1],
            packed_rotation[2],
            _pack_position(scale.x),
            _pack_position(scale.y),
            _pack_position(scale.z),
        ]

    def write_to_file(self, file, attributes: BoneAttributes | None = None):
        if not attributes or attributes.has_pos:
            file.write(struct.pack(">hhh",
                self._data[0], self._data[1], self._data[2]                
            ))

        if not attributes or attributes.has_rot:
            file.write(struct.pack(">hhh",
                self._data[3], self._data[4], self._data[5]                
            ))

        if not attributes or attributes.has_scale:
            file.write(struct.pack(">hhh",
                self._data[6], self._data[7], self._data[8]                
            ))


    def determine_needed_channels(self, default_pose: "PackedArmatureData", output: BoneAttributes):
        if default_pose._data[0] != self._data[0] or \
            default_pose._data[1] != self._data[1] or \
            default_pose._data[2] != self._data[2]:
            output.has_pos = True

        if default_pose._data[3] != self._data[3] or \
            default_pose._data[4] != self._data[4] or \
            default_pose._data[5] != self._data[5]:
            output.has_rot = True
        
        if default_pose._data[6] != self._data[6] or \
            default_pose._data[7] != self._data[7] or \
            default_pose._data[8] != self._data[8]:
            output.has_scale = True

class PackedAnimation():
    def __init__(self):
        self._frames: list[list[PackedArmatureData]] = []

    def add_frame(self, frame: list[PackedArmatureData]):
        self._frames.append(frame)

    def frame_count(self):
        return len(self._frames)

    def determine_needed_channels(self, default_pose: list[PackedArmatureData]) -> list[BoneAttributes]:
        result: list[BoneAttributes] = [BoneAttributes() for _ in default_pose]

        for bone_idx, defualt_bone_pose in enumerate(default_pose):
            bone_attr = result[bone_idx]

            for frame in self._frames:
                frame[bone_idx].determine_needed_channels(defualt_bone_pose, bone_attr)

        return result
    
    def write_to_file(self, file, attributes: list[BoneAttributes]):
        for frame in self._frames:
            for bone_idx, bone_pose in enumerate(frame):
                bone_pose.write_to_file(file, attributes[bone_idx])


class ArmatureBone:
    def __init__(self, index: int, bone: bpy.types.Bone, pose_bone: bpy.types.PoseBone, armature_transform: mathutils.Matrix):
        self.index: int = index
        self.name: str = bone.name
        self.parent_names: list[str] = []

        curr = bone.parent

        while curr:
            self.parent_names.append(curr.name)
            curr = curr.parent

        self.matrix_world: mathutils.Matrix = armature_transform @ bone.matrix_local
        self.matrix_world_inv: mathutils.Matrix = self.matrix_world.inverted()
        self.matrix_normal_inv = self.matrix_world_inv.to_3x3().inverted().transposed()

        self.pose_bone: bpy.types.Bone = pose_bone

class ArmatureData:
    def __init__(self, obj: bpy.types.Object, base_transform: mathutils.Matrix):
        self.relative_vertex_transform: mathutils.Matrix = base_transform @ obj.matrix_world
        self.relative_bone_transform: mathutils.Matrix = base_transform @ obj.matrix_world.inverted()
        self.armature: bpy.types.Armature = obj.data
        self.obj: bpy.types.Object = obj
        self.used_bones: set[str] = set()

        self._filtered_bones: dict[str, ArmatureBone] = {}
        self._ordered_bones: list[ArmatureBone] = []

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
        for idx, bone in enumerate(self.armature.bones):
            if bone.name not in self.used_bones:
                continue

            self._filtered_bones[bone.name] = ArmatureBone(
                len(self._filtered_bones),
                bone,
                self.obj.pose.bones[idx],
                self.relative_vertex_transform
            )

        self._ordered_bones = sorted(list(self._filtered_bones.values()), key = lambda x: x.index)

    def find_bone_data(self, bone_name: str):
        return self._filtered_bones[bone_name]
    
    def find_parent_bone(self, bone_name: str):
        curr = self.find_bone_data(bone_name)

        for parent_name in curr.parent_names:
            if parent_name in self.used_bones:
                return self._filtered_bones[parent_name]
            
        return None
    
    def get_filtered_bones(self) -> list[ArmatureBone]:
        return self._ordered_bones
    
    def is_action_compatible(self, action: bpy.types.Action) -> bool:
        if len(action.groups) == 0:
            return False

        for group in action.groups:
            if not group.name in self.armature.bones:
                return False
            
        return True
    
    def generate_pose_data(self) -> list[PackedArmatureData]:
        result: list[PackedArmatureData] = []
        for bone in self._ordered_bones:      
            parent = self.find_parent_bone(bone.name)

            transform = bone.pose_bone.matrix

            if parent:
                transform = parent.pose_bone.matrix.inverted() @ transform 
            else: 
                transform = self.relative_bone_transform @ transform

            loc, rot, scale = transform.decompose()

            result.append(PackedArmatureData(loc, rot, scale))
        return result
    
def determine_attribute_size(attributes: BoneAttributes) -> int:
    result = 0

    if attributes.has_pos:
        result += 6
    if attributes.has_rot:
        result += 6
    if attributes.has_scale:
        result += 6

    return result

def determine_attribute_list_size(attributes: list[BoneAttributes]) -> int:
    result = 0

    for attr in attributes:
        result += determine_attribute_size(attr)

    return result

def determine_used_attributes(bones: list[ArmatureBone], action: bpy.types.Action) -> list[BoneAttributes]:
    result: list[BoneAttributes] = []

    for bone in bones:
        if not bone.name in action.groups:
            result.append(BoneAttributes())
            continue

        group = action.groups[bone.name]

        attrs = BoneAttributes()

        for channel in group.channels:
            attr_name = channel.data_path.split('.')

            if len(attr_name) != 3:
                continue

            if attr_name[2] == 'location':
                attrs.has_pos = True

            if attr_name[2] == 'rotation_quaternion':
                attrs.has_rot = True
                
            if attr_name[2] == 'scale':
                attrs.has_scale = True

        result.append(attrs)

    return result

def get_channels(group: bpy.types.ActionGroup, suffix: str) -> list[bpy.types.FCurve]:
    result = []

    for channel in group.channels:
        if channel.data_path.endswith(suffix):
            result.append(channel)

    result = sorted(result, key=lambda x: x.array_index)

    return result

def _pack_position(input: float) -> int:
    return int(input * 256 + 0.5)

def _pack_quaternion(input: mathutils.Quaternion):
    if input.w < 0:
        final_input = -input
    else:
        final_input = input

    return [
        int(32767 * final_input.x + 0.5),
        int(32767 * final_input.y + 0.5),
        int(32767 * final_input.z + 0.5)
    ]