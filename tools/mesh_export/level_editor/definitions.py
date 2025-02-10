
import bpy
from bpy.path import abspath
import os.path
import json
from ..parse.struct_parse import find_structs, find_enums

class Definitions:
    def __init__(self):
        self.definitions = None
        self.enums = None
        self.loaded_path = None
        self.materials = []
        self.repo_path = None
        self.objects = None
        self.scripts = []

        self._objects_for_library_path = {}

    def _search_blend_files(self, start_path):
        result = []
        curr_path = abspath("//")

        for dirpath, dirnames, filenames in os.walk(start_path):
            for filename in filenames:
                if not filename.endswith('.blend'):
                    continue

                abs_path = os.path.join(dirpath, filename)

                if abs_path == bpy.data.filepath:
                    continue

                relative_path = os.path.relpath(abs_path, curr_path)

                result.append(relative_path)

        return result


    def _search_scripts(self, start_path):
        result = []

        for dirpath, dirnames, filenames in os.walk(start_path):
            for filename in filenames:
                if not filename.endswith('.script'):
                    continue

                abs_path = os.path.join(dirpath, filename)

                if abs_path == bpy.data.filepath:
                    continue

                relative_path = os.path.relpath(abs_path, start_path)

                result.append(f"rom:/{relative_path}")

        return result


    def load(self):
        current_path = bpy.data.filepath

        if current_path == self.loaded_path:
            return
        
        repo_path = self._find_repo_path()

        print("reloading from ", self.loaded_path, " to ", current_path)

        self.loaded_path = current_path
        self.repo_path = repo_path

        if not repo_path:
            print("No repo path found")
            return

        self._objects_for_library_path = {}
        
        with open(os.path.join(repo_path, "assets/game_objects.json"), "r") as file:
            self.objects = json.load(file)

        for obj in self.objects["objects"]:
            library_path_parts = obj['mesh'].split('#', 1)

            library_path = os.path.join(repo_path, "assets", library_path_parts[0])
            relative_path = '//' + os.path.relpath(library_path, os.path.dirname(bpy.data.filepath))
            obj['library'] = relative_path
            obj['mesh_name'] = library_path_parts[1]

            self._objects_for_library_path[relative_path] = obj


        print("Found repo path at " + str(repo_path))

        scene_config_path = os.path.join(repo_path, 'src/scene/scene_definition.h')

        if not scene_config_path:
            return
        
        with open(scene_config_path, 'r') as file:
            file_contents = file.read()
            self.definitions = find_structs(file_contents)
            self.enums = find_enums(file_contents)

            print("Found definitions " , self.definitions.keys())

        siblings = []
        siblings = self._search_blend_files(os.path.join(repo_path, 'assets/scenes'))

        self.materials = list(filter(lambda x: x.endswith('.mat.blend'), self._search_blend_files(os.path.join(repo_path, 'assets/materials'))))

        self.scripts = self._search_scripts(os.path.join(repo_path, 'assets'))

        # TODO search for target locations
        # for sibling in siblings:
        #     with bpy.data.libraries.load("//" + sibling, link=True) as (data_from, data_to):
        #         print(data_from.objects)


    def _find_repo_path(self):
        curr = abspath("//")

        while not os.path.exists(os.path.join(curr, 'src/scene/scene_definition.h')):
            next = os.path.dirname(curr)

            if next == curr:
                return None
            else:
                curr = next
        
        return curr
    
    def needs_reload(self):
        return bpy.data.filepath != self.loaded_path

    def get_structure_type(self, name):
        def_type_name = f"{name}_definition" 

        if self.needs_reload():
            self.load()

        if not self.definitions:
            return None

        if not def_type_name in self.definitions:
            return None
        
        return self.definitions[def_type_name]
    
    def get_materials(self):
        self.load()
        return self.materials
    
    def get_repo_path(self):
        self.load()
        return self.repo_path
    
    def get_objects_list(self):
        self.load()
        return self.objects["objects"]

    def get_object_for_library_path(self, path):
        self.load()

        if not path.startswith('//'):
            path = '//' + os.path.relpath(path, os.path.dirname(bpy.data.filepath))

        if path in self._objects_for_library_path:
            return self._objects_for_library_path[path]

        return None
    
    def get_struct_info(self, type):
        self.load()

        key = type + '_definition'

        if key in self.definitions:
            return self.definitions[key]
        
        return None
    
    def get_enum(self, enum_name):
        self.load()

        if enum_name in self.enums:
            return self.enums[enum_name]
        
        return None
    
    def get_scripts(self):
        self.load()
        return self.scripts


object_definitions = Definitions()