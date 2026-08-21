import bpy

SKIPPED_MODIFIERS = {
    "ARMATURE",
}

def apply_modifiers(obj: bpy.types.Object):
    bpy.context.view_layer.objects.active = obj

    for modifier in obj.modifiers:
        if modifier.type in SKIPPED_MODIFIERS or not modifier.show_render:
            print("skipping modifier " + modifier.type)
            continue
        bpy.ops.object.modifier_apply(modifier=modifier.name, single_user = True)