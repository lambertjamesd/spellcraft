import bpy
import sys
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.material_extract
import mesh_export.entities.serialize

from mesh_export.deps import generate_deps

if __name__ == "__main__":
    output_directory = sys.argv[-1]

    generate_deps.generate_deps(output_directory, os.path.relpath(__file__))

    for material in bpy.data.materials:
        if not 'f3d_mat' in material or not 'rdp_settings' in material['f3d_mat']:
            continue
        
        if output_directory.endswith('.mat'):
            output_filename= output_directory
        else:
            output_filename = os.path.join(output_directory, material.name) + '.mat'

        print(f'Writing material to {output_filename}')
        result = mesh_export.entities.material_extract.determine_material_from_f3d(material)
        print(result)
        os.makedirs(os.path.dirname(output_filename), exist_ok=True)
        mesh_export.entities.serialize.serialize_material(output_filename, result)
