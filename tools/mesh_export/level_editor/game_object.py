import bpy
from .definitions import object_definitions
import os

class NODE_OT_game_object_add(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_add"
    bl_label = "Add a game object"
    bl_description = "Add a game object"

    name: bpy.props.StringProperty()
    filepath: bpy.props.StringProperty(
        subtype='FILE_PATH',
    )

    def search_for_mesh(self, absolute_path, relative_path, mesh_name):
        for mesh in bpy.data.meshes:
            if mesh.name == mesh_name and mesh.library and (mesh.library.filepath == absolute_path or mesh.library.filepath == relative_path):
                return mesh
        
        return None
    
    def get_or_link_mesh(self, absolute_path, mesh_name):
        relative_path = "//" + os.path.relpath(absolute_path, os.path.dirname(bpy.data.filepath))

        result = self.search_for_mesh(absolute_path, relative_path, mesh_name)

        if result:
            return result

        with bpy.data.libraries.load(relative_path, link=True) as (data_from, data_to):
            data_to.meshes = [mesh_name]

        return self.search_for_mesh(absolute_path, relative_path, mesh_name)

    def execute(self, context):
        parts = self.filepath.split('#', 1)
        absolute_path = os.path.join(object_definitions.get_repo_path(), parts[0])

        mesh = self.get_or_link_mesh(absolute_path, parts[1])

        if not mesh:
            return {'ERROR'}

        object = bpy.data.objects.new(self.name, mesh)

        object.location = context.scene.cursor.location
        bpy.context.collection.objects.link(object)
        bpy.context.view_layer.objects.active = object

        return {'FINISHED'}

class NODE_MT_game_object_add(bpy.types.Menu):
    bl_label = "Game Object"

    def draw(self, context):
        object_definitions.load()
        layout = self.layout

        for game_object_def in object_definitions.get_objects_list():
            props = layout.operator(
                NODE_OT_game_object_add.bl_idname,
                text=game_object_def["name"],
            )
            props.filepath = os.path.join("assets", game_object_def["mesh"])
            props.name = game_object_def["name"]

def menu_function(self, context):
    self.layout.menu(
        NODE_MT_game_object_add.__name__,
        text="Game Object",
        icon='PLUGIN',
    )

def register():
    bpy.utils.register_class(NODE_MT_game_object_add)
    bpy.utils.register_class(NODE_OT_game_object_add)
    bpy.types.TOPBAR_MT_file.append(menu_function)

def unregister():
    bpy.utils.unregister_class(NODE_MT_game_object_add)
    bpy.utils.unregister_class(NODE_OT_game_object_add)
    bpy.types.TOPBAR_MT_file.remove(menu_function)