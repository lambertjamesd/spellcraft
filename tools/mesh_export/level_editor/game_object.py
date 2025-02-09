import bpy
from .definitions import object_definitions
import os
from ..parse import struct_parse

def _get_obj_def(current_object):
    if not current_object:
        return None

    if current_object.type != 'MESH':
        return None

    if not current_object.data or not current_object.data.library:
        return None
        
    return object_definitions.get_object_for_library_path(current_object.data.library.filepath)

def _init_default_properties(target):
    obj_def = _get_obj_def(target)

    structure = object_definitions.get_struct_info(obj_def['type'])

    if not structure:
        return

    for attr in structure.children:
        if attr.name == 'position' or \
            attr.name == 'rotation':
            continue
        
        if attr.data_type == 'bool':
            if not attr.name in target:
                target[attr.name] = False
        elif attr.data_type.startswith('enum '):
            if not attr.name in target:
                default_value = ''
                if attr.data_type in object_definitions.enums:
                    print('getting default for ', attr.data_type)
                    print(object_definitions.enums[attr.data_type])
                    default_value = object_definitions.enums[attr.data_type].int_to_str(0)

                target[attr.name] = default_value
                print('default_value', default_value)
        else:
            print('could not generate default for ' + attr.data_type)
            continue


class NODE_OT_game_object_init(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_init"
    bl_label = "Initialize game object properties"
    bl_description = "Sets up a game object's properties"

    def execute(self, context):
        if not context.object:
            return
        
        _init_default_properties(context.object)

        return {'FINISHED'}



    
class GameObjectPanel(bpy.types.Panel):
    bl_idname='GO_PT_game_object'
    bl_label='Game object'
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Game Object"


    @classmethod
    def poll(cls, context):
        obj_def = _get_obj_def(context.object)
        return obj_def != None
    
    def draw(self, context):
        obj_def = _get_obj_def(context.object)

        structure = object_definitions.get_struct_info(obj_def['type'])

        if not structure:
            return
        
        layout = self.layout
        layout.label(text=obj_def['name'])
        target = context.object

        is_mising_props = False

        for attr in structure.children:
            if attr.name == 'position' or \
                attr.name == 'rotation':
                continue

            if not attr.name in target:
                is_mising_props = True
                continue

            layout.prop(target, f'["{attr.name}"]')

        if is_mising_props:
            layout.operator('node.game_object_init', text='Add missing properties')

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
        _init_default_properties(object)

        object.location = context.scene.cursor.location
        bpy.context.collection.objects.link(object)
        object.select_set(True)
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

class CreateGameObjectPanel(bpy.types.Panel):
    bl_idname = "GO_PT_create_game_object"
    bl_label = "Create game object"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Game Object"

    @classmethod
    def poll(cls, context):
        return object_definitions.get_repo_path() != None
    
    def draw(self, context):
        col = self.layout.column()
        col.menu(NODE_MT_game_object_add.__name__, text="Create game object")

def register():
    bpy.utils.register_class(NODE_MT_game_object_add)
    bpy.utils.register_class(NODE_OT_game_object_add)
    bpy.utils.register_class(NODE_OT_game_object_init)
    bpy.utils.register_class(GameObjectPanel)
    bpy.utils.register_class(CreateGameObjectPanel)
    bpy.types.TOPBAR_MT_file.append(menu_function)

def unregister():
    bpy.utils.unregister_class(NODE_MT_game_object_add)
    bpy.utils.unregister_class(NODE_OT_game_object_add)
    bpy.utils.unregister_class(NODE_OT_game_object_init)
    bpy.utils.unregister_class(GameObjectPanel)
    bpy.utils.unregister_class(CreateGameObjectPanel)
    bpy.types.TOPBAR_MT_file.remove(menu_function)
