import bpy

enumSurfaceType = [
    ("default", "Default", ""),
    ("holey", "Holey", ""),
    ("water", "Water", ""),
    ("sticky", "Sticky", ""),
    ("slippery", "Slippery", ""),
    ("hurt_vine", "Hurt vine", ""),
]

enumMicrocode = [
    ("t3d", "Tiny 3D", ""),
    ("menu", "Menu", ""),
]

# matches TexScrollType in entities/material.py
enumTexScrollType = [
    ("NONE", "None", "No scrolling"),
    ("LINEAR", "Linear", "Scroll from start to end over time"),
    ("EASE_IN", "Ease In", ""),
    ("EASE_OUT", "Ease Out", ""),
    ("EASE_IN_OUT", "Ease In and Out", ""),
]

class TexScrollAxisProperties(bpy.types.PropertyGroup):
    type: bpy.props.EnumProperty(name="Type", items=enumTexScrollType, default="NONE")
    start: bpy.props.FloatProperty(name="Start", default=0)
    end: bpy.props.FloatProperty(name="End", default=0)
    time: bpy.props.FloatProperty(name="Time", default=0, min=0)
    phase_shift: bpy.props.FloatProperty(name="Phase Shift", default=0, min=0, max=1)
    pingpong: bpy.props.BoolProperty(name="Ping Pong", default=False, description="Reverse direction at the end instead of looping")

class TexScrollProperties(bpy.types.PropertyGroup):
    s: bpy.props.PointerProperty(type=TexScrollAxisProperties)
    t: bpy.props.PointerProperty(type=TexScrollAxisProperties)

class MATERIAL_PT_spellcraft_collision(bpy.types.Panel):
    bl_label = "Collision Inspector"
    bl_idname = "MATERIAL_PT_spellcraft_collision"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def _draw_axis(self, layout, axis, label):
        box = layout.box()
        split = box.split(factor=0.5)
        split.label(text=label)
        split.prop(axis, "type", text="")

        if axis.type == "NONE":
            return

        col = box.column(align=True)
        col.prop(axis, "start")
        col.prop(axis, "end")
        col.prop(axis, "time")
        col.prop(axis, "phase_shift")
        col.prop(axis, "pingpong")

    def _draw_tex(self, layout, tex_scroll, label):
        col = layout.column()
        col.label(text=label)
        self._draw_axis(col, tex_scroll.s, "S")
        self._draw_axis(col, tex_scroll.t, "T")

    def draw(self, context):
        box = self.layout.box()
        material = context.material
        split = box.split(factor=0.5)
        split.label(text="Surface Type")
        split.prop(material, "surface_type", text="")
        
        split = box.split(factor=0.5)
        split.label(text="Microcode")
        split.prop(material, "microcode", text="")
        
        self._draw_tex(self.layout, material.tex_scroll_0, "Texture 0")
        self._draw_tex(self.layout, material.tex_scroll_1, "Texture 1")

def register():
    bpy.utils.register_class(TexScrollAxisProperties)
    bpy.utils.register_class(TexScrollProperties)
    bpy.types.Material.surface_type = bpy.props.EnumProperty(
        name="Surface Type", items=enumSurfaceType, default="default"
    )
    bpy.types.Material.microcode = bpy.props.EnumProperty(
        name="Surface Type", items=enumMicrocode, default="t3d"
    )
    bpy.types.Material.tex_scroll_0 = bpy.props.PointerProperty(type=TexScrollProperties)
    bpy.types.Material.tex_scroll_1 = bpy.props.PointerProperty(type=TexScrollProperties)
    bpy.utils.register_class(MATERIAL_PT_spellcraft_collision)

def unregister():
    bpy.utils.unregister_class(MATERIAL_PT_spellcraft_collision)
    del bpy.types.Material.surface_type
    del bpy.types.Material.microcode
    del bpy.types.Material.tex_scroll_0
    del bpy.types.Material.tex_scroll_1
    bpy.utils.unregister_class(TexScrollProperties)
    bpy.utils.unregister_class(TexScrollAxisProperties)
