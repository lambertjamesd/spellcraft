import bpy

from .definitions import object_definitions

def _get_music(self, context):
    result = list(map(lambda x: (x, x, ''), object_definitions.get_music()))
    result.insert(0, ("none", "none", ""))
    return result

enumLightSource = [
    ("none", "None", ""),
    ("from_camera", "From camera", ""),
    ("rim", "Rim light", ""),
]

class SCENE_PT_spellcraft_settings(bpy.types.Panel):
    bl_label = "Scene inspector"
    bl_idname = "SCENE_PT_spellcraft_settings"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        return context.scene is not None

    def draw(self, context):
        box = self.layout.box()
        scene = context.scene
        split = box.split(factor=0.5)
        split.label(text="Default material")
        split.prop(scene, "default_material", text="")

        split = box.split(factor=0.5)
        split.label(text="Light source")
        split.prop(scene, "light_source", text="")

        split = box.split(factor=0.5)
        split.label(text="Fog color")
        split.prop(scene, "fog_color", text="")
        
        split = box.split(factor=0.5)
        split.label(text="Fog range")
        split.prop(scene, "fog_range", text="")
        
        split = box.split(factor=0.5)
        split.label(text="Music")
        split.prop(scene, "music", text="")

        split = box.split(factor=0.5)
        split.label(text="Clear color")
        split.prop(scene, "clear_color", text="")

def register():
    bpy.types.Scene.default_material = bpy.props.PointerProperty(
        name="Default material",
        type=bpy.types.Material,
        description="Select a material"
    )
    bpy.types.Scene.light_source = bpy.props.EnumProperty(
        name="Light source", items=enumLightSource, default="none"
    )
    bpy.types.Scene.music = bpy.props.EnumProperty(
        name="Music", items=_get_music
    )
    bpy.types.Scene.clear_color = bpy.props.FloatVectorProperty(
        name="Background color",
        description="Sets the framebuffer clear color",
        subtype="COLOR",
        size=4,
        min=0,
        max=1
    )
    bpy.utils.register_class(SCENE_PT_spellcraft_settings)

def unregister():
    bpy.utils.unregister_class(SCENE_PT_spellcraft_settings)
    del bpy.types.Scene.default_material
    del bpy.types.Scene.light_source
    del bpy.types.Scene.music
    del bpy.types.Scene.clear_color
