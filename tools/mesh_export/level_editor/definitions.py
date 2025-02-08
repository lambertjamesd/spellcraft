
import bpy
from bpy.path import abspath
import os.path
from ..parse.struct_parse import find_structs, find_enums

class Definitions:
    def __init__(self):
        self.definitions = None
        self.enums = None
        self.loaded_path = None
        self.materials = []
        self.repo_path = None

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

                print(abs_path, relative_path)

                result.append(relative_path)

        return result


    def load(self):
        current_path = bpy.data.filepath
        repo_path = self._find_repo_path()

        if current_path == self.loaded_path:
            return

        print("reloading from ", self.loaded_path, " to ", current_path)

        self.loaded_path = current_path
        self.repo_path = repo_path

        if not repo_path:
            print("No repo path found")
            return

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

        for sibling in siblings:
            with bpy.data.libraries.load("//" + sibling) as (data_from, data_to):
                print(data_from)

        print("found siblings", siblings)
        print("found materials", self.materials)

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

object_definitions = Definitions()