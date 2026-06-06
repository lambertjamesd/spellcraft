import bpy
import sys
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.material_extract
import mesh_export.entities.serialize
import mesh_export.entities.material
import mesh_export.entities.material_delta

from mesh_export.deps import generate_deps

if __name__ == "__main__":
    output_directory = sys.argv[-1]

    generate_deps.generate_deps(output_directory, os.path.relpath(__file__))
    
    default_material = mesh_export.entities.material.Material()
    default_material.vtx_effect = mesh_export.entities.material.VtxEffect(mesh_export.entities.material.VtxEffectType.VTX_EFFECT_NONE)

    did_export = None

    for material in bpy.data.materials:
        f3d_mat = None

        if 'f3d_mat' in material:
            f3d_mat = material['f3d_mat']
        elif hasattr(material, 'f3d_mat') and material.is_f3d:
            f3d_mat = material.f3d_mat

        if not f3d_mat or (not 'rdp_settings' in f3d_mat and not hasattr(f3d_mat, 'rdp_settings')):
            print(f'skipping material {material.name}')
            continue

        if did_export:
            raise Exception(f'found two materials {did_export} and {material.name}')
        
        if output_directory.endswith('.mat'):
            output_filename= output_directory
        else:
            output_filename = os.path.join(output_directory, material.name) + '.mat'

        print(f'Writing material to {output_filename}')
        with open(output_filename, 'wb') as output:
            result = mesh_export.entities.material_extract.determine_material_from_f3d(material)
            print(result)
            os.makedirs(os.path.dirname(output_filename), exist_ok=True)
            mesh_export.entities.serialize.serialize_material_file(output, result)

            revert = mesh_export.entities.material_delta.determine_material_delta(result, default_material)
            mesh_export.entities.serialize.serialize_material_file(output, revert)

            did_export = material.name

    if not did_export:
        raise Exception('no material found to export')
