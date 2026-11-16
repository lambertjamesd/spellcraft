import os
import bpy

# path of the blend file being exported relative to the working directory.
# Exporters expect the working directory to be the project root so this
# starts with assets/. Derived from bpy.data.filepath instead of sys.argv so
# it also works when exporting from a running blender instance
def source_filename() -> str:
    return os.path.relpath(bpy.data.filepath)

def rom_filename(library_path: str) -> str:
    input_filename = source_filename()
    image_path = os.path.normpath(os.path.join(os.path.dirname(input_filename), library_path[2:]))
    # paths use \ on windows but rom paths always use /
    image_path = image_path.replace('\\', '/')

    assets_index = image_path.index('assets/')
    return 'rom:/' + image_path[assets_index + len('assets/'):-len('.blend')]
