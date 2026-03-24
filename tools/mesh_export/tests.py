import sys
import os.path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mathutils
import io
import mesh_export.entities.mesh_collider

collider = mesh_export.entities.mesh_collider.MeshCollider()

collider.vertices.append(mathutils.Vector((-10, -10, -10)))
collider.vertices.append(mathutils.Vector((10, -10, 10)))
collider.vertices.append(mathutils.Vector((0, 10, 0)))

collider.triangles.append(mesh_export.entities.mesh_collider.MeshColliderTriangle([0, 1, 2]))

file_tmp = io.BytesIO()
collider.write_out(file_tmp)