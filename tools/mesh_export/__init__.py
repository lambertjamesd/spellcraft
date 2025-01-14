
bl_info = {
    "name": "Level Editor",
    "version": (1, 0, 0),
    "description": "Plugin for making blender work better as a level editor",
    "blender": (4, 0, 0),
    "category": "Object",
}

import bpy
from bpy.types import Panel, Mesh
from bpy.path import abspath
import os.path
from .parse.struct_parse import find_structs, find_enums

class Definitions:
    def __init__(self):
        self.definitions = None
        self.enums = None
        self.has_loaded = False

    def load(self):
        print("Loading")
        if self.has_loaded:
            return

        self.has_loaded = True
        scene_config_path = self._find_scene_config()
        print("Found scene config path " + str(scene_config_path))

        if not scene_config_path:
            return
        
        with open(scene_config_path, 'r') as file:
            file_contents = file.read()
            self.definitions = find_structs(file_contents)
            self.enums = find_enums(file_contents)

            print("Found definitions " , self.definitions.keys())

    def _find_scene_config(self):
        curr = abspath("//")

        while not os.path.exists(os.path.join(curr, 'src/scene/scene_definition.h')):
            next = os.path.dirname(curr)

            if next == curr:
                return None
            else:
                curr = next

        return os.path.join(curr, 'src/scene/scene_definition.h')

    def get_structure_type(self, name):
        def_type_name = f"{name}_definition" 

        if not self.has_loaded:
            self.load()

        if not self.definitions:
            return None

        if not def_type_name in self.definitions:
            return None
        
        return self.definitions[def_type_name]

_object_definitions = Definitions()

class GameObjectPropertiesPanel(Panel):
    bl_label = "Game object properties"
    bl_idname = "OBJECT_PT_SPELLCRAFT_GameObject"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        _object_definitions.load()
        return (
            context.object is not None
            and isinstance(context.object.data, Mesh)
            and 'type' in context.object.data
        )
    
    def draw(self, context):
        col = self.layout.column().box()

        structure_type = _object_definitions.get_structure_type(context.object.data['type'])

        col.box().label(text="Game object " + str(structure_type))


def register():
    bpy.utils.register_class(GameObjectPropertiesPanel)

def unregister():
    bpy.utils.unregister_class(GameObjectPropertiesPanel)

if __name__ == "__main__":
    register()