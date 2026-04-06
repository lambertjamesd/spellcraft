import bpy
import mathutils
import io
import struct

def _build_edge_mapping(input: bpy.types.Mesh) -> dict[int, list[int]]:
    result: dict[int, list[int]] = {}

    for edge in input.edges:
        for vtx_index in edge.vertices:
            if vtx_index in result:
                result[vtx_index].append(edge.index)
            else:
                result[vtx_index] = [edge.index]

    return result

def _write_edge_vtx(result: io.BytesIO, vtx_to_edge: dict[int, list[int]], vtx_index: int, edge_index: int):
    edge_list = vtx_to_edge[vtx_index]
    next_index = edge_list.index(edge_index) + 1
    if next_index == len(edge_list):
        next_index = 0
    result.write(struct.pack('>BB', vtx_index, next_index))

def build_line_mesh(input: bpy.types.Object, obj_transform: mathutils.Matrix) -> bytes:
    result = io.BytesIO()
    mesh = input.data

    if not isinstance(mesh, bpy.types.Mesh):
        result.write(b'\0\0')
        return result.getvalue()
    
    point_count = len(mesh.vertices)
    edge_count = len(mesh.edges)

    result.write(point_count.to_bytes(1, 'big'))
    result.write(edge_count.to_bytes(1, 'big'))

    vtx_to_edge = _build_edge_mapping(mesh)

    for vtx in mesh.vertices:
        transformed = obj_transform @ vtx.co
        result.write(struct.pack('>fff', transformed.x, transformed.y, transformed.z))

    for edge in mesh.edges:
        for vtx_index in edge.vertices:
            _write_edge_vtx(result, vtx_to_edge, vtx_index, edge.index)

    return result.getvalue()