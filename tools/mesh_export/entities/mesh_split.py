import mathutils
from . import mesh

def split(mesh: mesh.mesh_data, normal: mathutils.Vector, d: float) -> (mesh.mesh_data | None, mesh.mesh_data | None):
    behind: mesh.mesh_data = mesh.mesh_data(mesh.mat)
    front: mesh.mesh_data = mesh.mesh_data(mesh.mat)

    plane_distance = list(map(lambda x: normal.dot(x) + d, mesh.vertices))

    return behind, front