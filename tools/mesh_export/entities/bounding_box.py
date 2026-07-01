import mathutils
import bpy

def union(a: tuple[mathutils.Vector, mathutils.Vector], b: tuple[mathutils.Vector, mathutils.Vector]) -> tuple[mathutils.Vector, mathutils.Vector]:
    return mathutils.Vector((
        min(a[0].x, b[0].x),
        min(a[0].y, b[0].y),
        min(a[0].z, b[0].z)
    )), mathutils.Vector((
        max(a[1].x, b[1].x),
        max(a[1].y, b[1].y),
        max(a[1].z, b[1].z)
    ))

def from_vertices(points: list[mathutils.Vector]) -> tuple[mathutils.Vector, mathutils.Vector]:
    if len(points) == 0:
        return mathutils.Vector(), mathutils.Vector()
    
    min = points[0]
    max = points[0]

    for vtx in points:
        min, max = union((min, max), (vtx, vtx))

    return min, max

def from_obj(obj: bpy.types.Object) -> tuple[mathutils.Vector, mathutils.Vector]:
    final_transform = obj.matrix_world
    return from_vertices([final_transform @ mathutils.Vector(vtx) for vtx in obj.bound_box])