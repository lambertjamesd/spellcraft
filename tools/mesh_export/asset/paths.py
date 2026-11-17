import os

ASSETS_DIR_NAME = 'assets'

class AssetPaths():
    def __init__(self, filename: str):
        filename = os.path.abspath(filename)
        assets_dir = find_assets_dir(filename)

        # assets/ is a sibling of build/ and filesystem/
        self.root = os.path.dirname(assets_dir)
        relative_dir = os.path.dirname(os.path.relpath(filename, assets_dir))

        self.base_name = os.path.splitext(os.path.basename(filename))[0]
        self.build_dir = os.path.join(self.root, 'build', 'assets', relative_dir)
        self.output_dir = os.path.join(self.root, 'filesystem', relative_dir)

def find_assets_dir(filename: str) -> str:
    curr = os.path.dirname(filename)

    while True:
        if os.path.basename(curr) == ASSETS_DIR_NAME:
            return curr

        parent = os.path.dirname(curr)

        if parent == curr:
            raise Exception(f"{filename} is not inside an {ASSETS_DIR_NAME}/ folder")

        curr = parent

# returns true if filename is a blend file that is exported as a tmesh
def is_tmesh_source(filename: str) -> bool:
    if not filename.endswith('.blend') or filename.endswith(MATERIAL_BLEND_EXT):
        return False

    try:
        assets_dir = find_assets_dir(os.path.abspath(filename))
    except Exception:
        return False

    relative = os.path.relpath(os.path.abspath(filename), assets_dir)
    return relative.replace(os.sep, '/').startswith('meshes/')

MATERIAL_BLEND_EXT = '.mat.blend'

# returns true if filename is a blend file that is exported as a fast64 material
def is_material_source(filename: str) -> bool:
    if not filename.endswith(MATERIAL_BLEND_EXT):
        return False

    try:
        find_assets_dir(os.path.abspath(filename))
    except Exception:
        return False

    return True
