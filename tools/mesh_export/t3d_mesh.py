import bpy
import sys
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.asset.t3d_mesh

# the export logic lives in asset/t3d_mesh.py so the blender plugin can run it
# directly after saving. mkasset is passed in with the MK_ASSET environment variable
mesh_export.asset.t3d_mesh.process_scene(bpy.data.filepath)
