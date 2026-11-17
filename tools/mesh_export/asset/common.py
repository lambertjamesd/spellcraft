import bpy
import os
import os.path
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.material_extract

from mesh_export.asset.paths import MATERIAL_BLEND_EXT

# environment variable holding the path to libdragon's mkasset
MK_ASSET_ENV = 'MK_ASSET'

def compress_asset(intermediate_filename: str, output_dir: str, window_size: int):
    mk_asset = os.environ.get(MK_ASSET_ENV)

    if mk_asset:
        subprocess.run([mk_asset, '-o', output_dir, '-w', str(window_size), intermediate_filename], check=True)
    else:
        print(f"{MK_ASSET_ENV} not set, copying {intermediate_filename} uncompressed")
        shutil.copy(intermediate_filename, os.path.join(output_dir, os.path.basename(intermediate_filename)))

# finds the .mat.blend libraries, .mat.json files and images used by the
# currently open blend file so the makefile can rebuild the asset when they change
def find_dependencies(root: str) -> list[str]:
    result: set[str] = set()

    def add(filepath: str):
        filepath = os.path.normpath(filepath)

        if not os.path.isfile(filepath):
            return

        try:
            filepath = os.path.relpath(filepath, root)
        except ValueError:
            # on a different drive than the project root on windows
            pass

        result.add(filepath.replace('\\', '/'))

    for library in bpy.data.libraries:
        library_path = bpy.path.abspath(library.filepath)

        if library_path.endswith(MATERIAL_BLEND_EXT):
            add(library_path)

    for image in bpy.data.images:
        if image.source not in {'FILE', 'SEQUENCE', 'TILED'} or not image.filepath:
            continue

        add(bpy.path.abspath(image.filepath, library=image.library))

    for mat in bpy.data.materials:
        if mat.name.startswith('materials/'):
            add(os.path.join(root, 'assets', f"{mat.name}.mat.json"))

        # flipbook textures pull in every numbered frame next to the image
        if 'flipbook_0' in mat and mat['flipbook_0'] and hasattr(mat, 'f3d_mat'):
            tex0 = mesh_export.entities.material_extract._get_safe(mat.f3d_mat, 'tex0')
            image = tex0['tex']

            if image and image.filepath:
                image_path = bpy.path.abspath(image.filepath, library=image.library)

                for frame in mesh_export.entities.material_extract._look_for_frames(os.path.normpath(image_path)):
                    add(frame)

    return sorted(result)
