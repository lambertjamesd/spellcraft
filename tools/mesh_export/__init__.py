
bl_info = {
    "name": "Level Editor",
    "version": (1, 0, 0),
    "description": "Plugin for making blender work better as a level editor",
    "blender": (4, 0, 0),
    "category": "Object",
}

def register():
    from .level_editor import link_materials
    from .level_editor import game_object
    from .level_editor import material
    from .level_editor import scene
    from .level_editor import mat_anim
    from .level_editor import armature
    link_materials.register()
    game_object.register()
    material.register()
    scene.register()
    mat_anim.register()
    armature.register()

def unregister():
    from .level_editor import link_materials
    from .level_editor import game_object
    from .level_editor import material
    from .level_editor import scene
    from .level_editor import mat_anim
    from .level_editor import armature
    link_materials.unregister()
    game_object.unregister()
    material.unregister()
    scene.unregister()
    mat_anim.unregister()
    armature.unregister()

if __name__ == "__main__":
    register()