import bpy

# Blender can't store a color ramp directly as a custom property, so the
# gradient lives on a hidden Blend texture datablock that the mesh points to.
# The ramp itself is texture.color_ramp.

class ColorSweepProperties(bpy.types.PropertyGroup):
    gradient: bpy.props.PointerProperty(
        name="Gradient",
        type=bpy.types.Texture,
        description="Texture holding the color ramp used for the sweep",
    )
    duration: bpy.props.FloatProperty(name="Duration", default=1, min=0)
    max_offset: bpy.props.FloatProperty(name="Max Offset", default=0, min=0)

class MESH_OT_spellcraft_color_sweep_add_gradient(bpy.types.Operator):
    bl_idname = "mesh.spellcraft_color_sweep_add_gradient"
    bl_label = "Add Gradient"
    bl_description = "Create a color ramp for the color sweep"
    bl_options = {"REGISTER", "UNDO"}

    @classmethod
    def poll(cls, context):
        return context.mesh is not None

    def execute(self, context):
        mesh = context.mesh
        texture = bpy.data.textures.new(name=f"{mesh.name}_color_sweep", type="BLEND")
        texture.use_color_ramp = True
        mesh.color_sweep.gradient = texture
        return {"FINISHED"}

class MESH_PT_spellcraft_color_sweep(bpy.types.Panel):
    bl_label = "Color sweep"
    bl_idname = "MESH_PT_spellcraft_color_sweep"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"
    bl_options = {"DEFAULT_CLOSED"}

    @classmethod
    def poll(cls, context):
        return context.mesh is not None

    def draw(self, context):
        layout = self.layout
        mesh = context.mesh
        color_sweep = mesh.color_sweep

        row = layout.row(align=True)
        row.prop(color_sweep, "gradient")
        if color_sweep.gradient is None:
            row.operator(MESH_OT_spellcraft_color_sweep_add_gradient.bl_idname, text="", icon="ADD")
        else:
            gradient = color_sweep.gradient
            if gradient.use_color_ramp:
                layout.template_color_ramp(gradient, "color_ramp", expand=True)
            else:
                layout.prop(gradient, "use_color_ramp", text="Enable color ramp")

        col = layout.column(align=True)
        col.prop(color_sweep, "duration")
        col.prop(color_sweep, "max_offset")

def register():
    bpy.utils.register_class(ColorSweepProperties)
    bpy.utils.register_class(MESH_OT_spellcraft_color_sweep_add_gradient)
    bpy.types.Mesh.color_sweep = bpy.props.PointerProperty(type=ColorSweepProperties)
    bpy.utils.register_class(MESH_PT_spellcraft_color_sweep)

def unregister():
    bpy.utils.unregister_class(MESH_PT_spellcraft_color_sweep)
    del bpy.types.Mesh.color_sweep
    bpy.utils.unregister_class(MESH_OT_spellcraft_color_sweep_add_gradient)
    bpy.utils.unregister_class(ColorSweepProperties)
