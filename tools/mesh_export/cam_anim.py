import sys

from mesh_export.entities import camera_animation
from mesh_export.cutscene import variable_layout

def process_scene():
    output_filename = sys.argv[-1]

    globals = variable_layout.VariableLayout()

    with open('build/assets/scripts/globals.json') as file:
        globals.deserialize(file)

    variable_context = variable_layout.VariableContext(globals, variable_layout.VariableLayout(), None)

    with open(output_filename, 'wb') as file:
        camera_animation.export_camera_animations(output_filename.replace('.canim', '.sanim'), file, variable_context)

process_scene()
