import bpy
import sys
import os
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.material_extract
import mesh_export.entities.serialize
import mesh_export.entities.material
import mesh_export.entities.material_delta

from mesh_export.deps import generate_deps
from mesh_export.asset.paths import AssetPaths
from mesh_export.asset.common import compress_asset, find_dependencies

# the .d.template next to the makefile entry script
DEPS_SCRIPT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'material_fast64.py'))

MATERIAL_WINDOW_SIZE = 4

def _get_f3d_mat(material: bpy.types.Material):
    if hasattr(material, 'f3d_mat') and material.is_f3d:
        return material.f3d_mat
    elif 'f3d_mat' in material:
        return material['f3d_mat']

    return None

def _find_material() -> bpy.types.Material:
    result = None

    for material in bpy.data.materials:
        f3d_mat = _get_f3d_mat(material)

        if not f3d_mat or (not 'rdp_settings' in f3d_mat and not hasattr(f3d_mat, 'rdp_settings')):
            print(f'skipping material {material.name}')
            continue

        if result:
            raise Exception(f'found two materials {result.name} and {material.name}')

        result = material

    if not result:
        raise Exception('no material found to export')

    return result

# exports the currently open .mat.blend file. filename is the path of the
# blend file and is used to find the build and filesystem directories
def process_material(filename: str):
    paths = AssetPaths(filename)

    # the exporters resolve assets/ paths relative to the working directory
    previous_cwd = os.getcwd()
    os.chdir(paths.root)

    try:
        _process_material(paths)
    finally:
        os.chdir(previous_cwd)

def _process_material(paths: AssetPaths):
    os.makedirs(paths.build_dir, exist_ok=True)
    os.makedirs(paths.output_dir, exist_ok=True)

    # base_name keeps the .mat from .mat.blend
    output_name = paths.base_name

    generate_deps.generate_deps(
        # generate_deps matches on filesystem/ so the separators have to be / on windows too
        os.path.relpath(os.path.join(paths.output_dir, output_name), paths.root).replace('\\', '/'),
        DEPS_SCRIPT,
        root=paths.root,
        dependencies=find_dependencies(paths.root)
    )

    default_material = mesh_export.entities.material.Material("Default")
    default_material.vtx_effect = mesh_export.entities.material.VtxEffect(mesh_export.entities.material.VtxEffectType.VTX_EFFECT_NONE)

    material = _find_material()

    # uncompressed intermediate material goes in the build directory
    intermediate_filename = os.path.join(paths.build_dir, output_name)

    print(f'Writing material {material.name} to {intermediate_filename}')

    result = mesh_export.entities.material_extract.determine_material_from_f3d(material)
    print(result)

    with open(intermediate_filename, 'wb') as output:
        mesh_export.entities.serialize.serialize_material_file(output, result)

        revert = mesh_export.entities.material_delta.determine_material_delta(result, default_material)
        mesh_export.entities.serialize.serialize_material_file(output, revert)

    # compress the material into the final output directory
    compress_asset(intermediate_filename, paths.output_dir, MATERIAL_WINDOW_SIZE)
