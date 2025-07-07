
bl_info = {
    "name": "Level Editor",
    "version": (1, 0, 0),
    "description": "Plugin for making blender work better as a level editor",
    "blender": (4, 0, 0),
    "category": "Object",
}

from .level_editor import link_materials
from .level_editor import game_object
from .level_editor import material

def register():
    link_materials.register()
    game_object.register()
    material.register()

def unregister():
    link_materials.unregister()
    game_object.unregister()
    material.unregister()

if __name__ == "__main__":
    register()