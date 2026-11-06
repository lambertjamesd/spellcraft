import bpy

class MatAnimProperties(bpy.types.PropertyGroup):
    has_prim: bpy.props.BoolProperty(name="Prim color", default=False)
    prim_color: bpy.props.FloatVectorProperty(
        name="Color",
        subtype="COLOR",
        size=4,
        min=0,
        max=1,
        default=(1, 1, 1, 1),
    )

    has_env: bpy.props.BoolProperty(name="Env color", default=False)
    env_color: bpy.props.FloatVectorProperty(
        name="Color",
        subtype="COLOR",
        size=4,
        min=0,
        max=1,
        default=(1, 1, 1, 1),
    )

    has_tex0_scroll: bpy.props.BoolProperty(name="tex0 scroll", default=False)
    tex0_scroll: bpy.props.FloatVectorProperty(
        name="Scroll",
        subtype="XYZ",
        size=2,
        default=(0, 0),
    )

    has_tex1_scroll: bpy.props.BoolProperty(name="tex1 scroll", default=False)
    tex1_scroll: bpy.props.FloatVectorProperty(
        name="Scroll",
        subtype="XYZ",
        size=2,
        default=(0, 0),
    )

class OBJECT_PT_mat_anim(bpy.types.Panel):
    bl_idname = "OBJECT_PT_mat_anim"
    bl_label = "Material animation"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"DEFAULT_CLOSED"}

    @classmethod
    def poll(cls, context):
        return context.object is not None

    def draw(self, context):
        layout = self.layout
        mat_anim = context.object.mat_anim

        layout.prop(mat_anim, "has_prim")
        if mat_anim.has_prim:
            layout.prop(mat_anim, "prim_color")

        layout.prop(mat_anim, "has_env")
        if mat_anim.has_env:
            layout.prop(mat_anim, "env_color")

        layout.prop(mat_anim, "has_tex0_scroll")
        if mat_anim.has_tex0_scroll:
            layout.column(align=True).prop(mat_anim, "tex0_scroll")

        layout.prop(mat_anim, "has_tex1_scroll")
        if mat_anim.has_tex1_scroll:
            layout.column(align=True).prop(mat_anim, "tex1_scroll")

def register():
    bpy.utils.register_class(MatAnimProperties)
    bpy.types.Object.mat_anim = bpy.props.PointerProperty(type=MatAnimProperties)
    bpy.utils.register_class(OBJECT_PT_mat_anim)

def unregister():
    bpy.utils.unregister_class(OBJECT_PT_mat_anim)
    del bpy.types.Object.mat_anim
    bpy.utils.unregister_class(MatAnimProperties)
