
bl_info = {
    "name": "Level Editor",
    "version": (1, 0, 0),
    "description": "Plugin for making blender work better as a level editor",
    "blender": (4, 0, 0),
    "category": "Object",
}

import bpy
from bpy.types import Panel, Mesh
from .level_editor import definitions
from .level_editor import link_materials
from .level_editor import game_object

class GameObjectPropertiesPanel(Panel):
    bl_label = "Game object properties"
    bl_idname = "OBJECT_PT_SPELLCRAFT_GameObject"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        definitions.object_definitions.load()
        return (
            context.object is not None
            and isinstance(context.object.data, Mesh)
            and 'type' in context.object.data
        )
    
    def draw(self, context):
        col = self.layout.column().box()

        structure_type = definitions.object_definitions.get_structure_type(context.object.data['type'])

        col.box().label(text="Game object " + str(structure_type))


def register():
    bpy.utils.register_class(GameObjectPropertiesPanel)
    link_materials.register()
    game_object.register()

def unregister():
    bpy.utils.unregister_class(GameObjectPropertiesPanel)
    link_materials.unregister()
    game_object.unregister()

if __name__ == "__main__":
    register()