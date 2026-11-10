import bpy

class ArmAnimProperties(bpy.types.PropertyGroup):
    event_attack: bpy.props.BoolProperty(name="Attack", default=False)
    event_step: bpy.props.BoolProperty(name="Step", default=False)
    event_can_recast: bpy.props.BoolProperty(name="Can recast", default=False)

class OBJECT_PT_arm_anim(bpy.types.Panel):
    bl_idname = "OBJECT_PT_arm_anim"
    bl_label = "Aramture animation"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"DEFAULT_CLOSED"}

    @classmethod
    def poll(cls, context):
        return context.object is not None and context.object.type == "ARMATURE"

    def draw(self, context):
        layout = self.layout
        arm_anim = context.object.arm_anim

        layout.prop(arm_anim, "event_attack")
        layout.prop(arm_anim, "event_step")
        layout.prop(arm_anim, "event_can_recast")

def register():
    bpy.utils.register_class(ArmAnimProperties)
    bpy.types.Object.arm_anim = bpy.props.PointerProperty(type=ArmAnimProperties)
    bpy.utils.register_class(OBJECT_PT_arm_anim)

def unregister():
    bpy.utils.unregister_class(OBJECT_PT_arm_anim)
    del bpy.types.Object.arm_anim
    bpy.utils.unregister_class(ArmAnimProperties)
