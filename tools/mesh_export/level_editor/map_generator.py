import bpy
import bmesh

NORMAL_Z_THRESHOLD = 0.780868809

def filter_mesh_by_normal(obj: bpy.types.Object, threshold=NORMAL_Z_THRESHOLD):
    if obj.type != 'MESH' or not isinstance(obj.data, bpy.types.Mesh):
        raise ValueError(f"{obj.name} is not a mesh")

    # Duplicate the object and its mesh
    new_obj = obj.copy()
    new_obj.data = obj.data.copy()
    new_obj.name = 'Map outline ' + obj.name

    # Link into the same collection(s)
    for collection in obj.users_collection:
        if new_obj.name not in collection.objects:
            collection.objects.link(new_obj)

    # Edit the duplicated mesh
    bm = bmesh.new()
    bm.from_mesh(new_obj.data)

    # Ensure normals are up-to-date
    bm.normal_update()

    # Delete faces whose Z normal is below the threshold
    faces_to_delete = [
        face for face in bm.faces
        if face.normal.z < threshold
    ]

    bmesh.ops.delete(
        bm,
        geom=faces_to_delete,
        context='FACES'
    )

    # Remove loose vertices
    loose_verts = [v for v in bm.verts if not v.link_faces]
    if loose_verts:
        bmesh.ops.delete(
            bm,
            geom=loose_verts,
            context='VERTS'
        )

    # Remove loose edges (normally there shouldn't be any after deleting
    # loose verts, but this handles edge-only geometry)
    loose_edges = [e for e in bm.edges if not e.link_faces]
    if loose_edges:
        bmesh.ops.delete(
            bm,
            geom=loose_edges,
            context='EDGES'
        )

    bm.to_mesh(new_obj.data)
    bm.free()

    new_obj.data.update()

    return new_obj