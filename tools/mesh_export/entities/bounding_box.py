import mathutils

def union(a: tuple[mathutils.Vector, mathutils.Vector], b: tuple[mathutils.Vector, mathutils.Vector]) -> tuple[mathutils.Vector, mathutils.Vector]:
    return mathutils.Vector((
        min(a.x, b.x),
        min(a.y, b.y),
        min(a.z, b.z)
    )), mathutils.Vector((
        max(a.x, b.x),
        max(a.y, b.y),
        max(a.z, b.z)
    ))