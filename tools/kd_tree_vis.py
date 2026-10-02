#!/usr/bin/env python3
"""
Scans a file for mesh collider data (CMSH header) and writes each kd tree
found to an obj file for visualization.

Each leaf is written as an object containing its triangles. Each branch is
written as an object containing its divider planes (a_max and b_min). Object
names encode the path taken from the root, for example:

    x_branch        root branch splitting on x
    x-y_branch      branch on y under the - (a) side of the root
    x-y+_leaf       leaf on the + (b) side of that branch

Usage: python3 tools/kd_tree_vis.py [-l] <file>

    -l  only export leaf objects, skipping branch planes

Outputs <file stem>_<index>.obj next to the input file for each CMSH found.
"""

import argparse
import math
import os
import struct
import sys

HEADER = b'CMSH'

KD_TREE_LEAF_NODE = 0
KD_TREE_BRANCH_NODE = 1

BRANCH_SIZE = 8
LEAF_SIZE = 4
TRIANGLE_SIZE = 8
VERTEX_SIZE = 12

AXIS_NAMES = ['x', 'y', 'z']

class MeshCollider():
    def __init__(self, min_point, size_inv, nodes, triangles, vertices):
        self.min_point = min_point
        self.size_inv = size_inv
        self.nodes: bytes = nodes
        self.triangles: list[tuple[int, int, int, int, int]] = triangles
        self.vertices: list[tuple[float, float, float]] = vertices

    def to_world(self, axis: int, value: int) -> float:
        return self.min_point[axis] + value / self.size_inv[axis]

def parse_mesh_collider(data: bytes, offset: int) -> MeshCollider | None:
    pos = offset + len(HEADER)

    if pos + 30 > len(data):
        return None

    min_point = struct.unpack_from('>fff', data, pos)
    size_inv = struct.unpack_from('>fff', data, pos + 12)
    node_size, triangle_count, vertex_count = struct.unpack_from('>HHH', data, pos + 24)
    pos += 30

    if not all(math.isfinite(v) for v in min_point) or \
            not all(math.isfinite(v) and v > 0 for v in size_inv):
        return None

    if node_size < LEAF_SIZE or node_size % LEAF_SIZE != 0:
        return None

    end = pos + node_size + triangle_count * TRIANGLE_SIZE + vertex_count * VERTEX_SIZE

    if end > len(data):
        return None

    nodes = data[pos:pos + node_size]
    pos += node_size

    triangles = []
    for _ in range(triangle_count):
        triangle = struct.unpack_from('>HHHBB', data, pos)
        if max(triangle[0:3]) >= vertex_count:
            return None
        triangles.append(triangle)
        pos += TRIANGLE_SIZE

    vertices = []
    for _ in range(vertex_count):
        vertices.append(struct.unpack_from('>fff', data, pos))
        pos += VERTEX_SIZE

    return MeshCollider(min_point, size_inv, nodes, triangles, vertices)

def find_mesh_colliders(data: bytes) -> list[MeshCollider]:
    result = []
    offset = data.find(HEADER)

    while offset != -1:
        collider = parse_mesh_collider(data, offset)

        if collider:
            result.append(collider)
        else:
            print(f'warning: CMSH at offset {offset} does not look like a valid mesh collider, skipping')

        offset = data.find(HEADER, offset + 1)

    return result

class ObjWriter():
    def __init__(self, collider: MeshCollider, leaves_only: bool = False):
        self.collider = collider
        self.leaves_only = leaves_only
        self.lines: list[str] = []
        self.vertex_count = 0

        for vertex in collider.vertices:
            self.add_vertex(vertex)

    def add_vertex(self, vertex) -> int:
        self.lines.append(f'v {vertex[0]:.6f} {vertex[1]:.6f} {vertex[2]:.6f}')
        self.vertex_count += 1
        # obj indices are 1 based
        return self.vertex_count

    def add_plane(self, axis: int, value: float, bb_min: list[float], bb_max: list[float]):
        u = (axis + 1) % 3
        v = (axis + 2) % 3
        corners = [
            (bb_min[u], bb_min[v]),
            (bb_max[u], bb_min[v]),
            (bb_max[u], bb_max[v]),
            (bb_min[u], bb_max[v]),
        ]

        indices = []
        for cu, cv in corners:
            point = [0.0, 0.0, 0.0]
            point[axis] = value
            point[u] = cu
            point[v] = cv
            indices.append(self.add_vertex(point))

        self.lines.append('f ' + ' '.join(str(i) for i in indices))

    def write_node(self, offset: int, path: str, bb_min: list[float], bb_max: list[float], depth: int = 0):
        nodes = self.collider.nodes

        if depth > 64:
            raise Exception(f'kd tree too deep at {path}, data is likely corrupt')

        if offset + LEAF_SIZE > len(nodes):
            raise Exception(f'node offset {offset} out of range at {path}')

        node_type = nodes[offset]

        if node_type == KD_TREE_LEAF_NODE:
            _, triangle_count, triangle_offset = struct.unpack_from('>BBH', nodes, offset)
            self.lines.append(f'o {path}_leaf')

            for i in range(triangle_offset, triangle_offset + triangle_count):
                if i >= len(self.collider.triangles):
                    print(f'warning: leaf {path} references triangle {i} out of range')
                    break
                a, b, c, _, _ = self.collider.triangles[i]
                self.lines.append(f'f {a + 1} {b + 1} {c + 1}')
            return

        if node_type != KD_TREE_BRANCH_NODE:
            raise Exception(f'unknown node type {node_type} at offset {offset} ({path})')

        _, axis, a_max, b_min, b_offset = struct.unpack_from('>BBHHH', nodes, offset)

        if axis > 2:
            raise Exception(f'invalid axis {axis} at offset {offset} ({path})')

        path += AXIS_NAMES[axis]
        a_max_world = self.collider.to_world(axis, a_max)
        b_min_world = self.collider.to_world(axis, b_min)

        if not self.leaves_only:
            self.lines.append(f'o {path}_branch')
            self.add_plane(axis, a_max_world, bb_min, bb_max)
            self.add_plane(axis, b_min_world, bb_min, bb_max)

        a_bb_max = list(bb_max)
        a_bb_max[axis] = a_max_world
        self.write_node(offset + BRANCH_SIZE, path + '-', bb_min, a_bb_max, depth + 1)

        b_bb_min = list(bb_min)
        b_bb_min[axis] = b_min_world
        self.write_node(offset + b_offset, path + '+', b_bb_min, bb_max, depth + 1)

    def write(self, filename: str):
        bb_min = [self.collider.to_world(axis, 0) for axis in range(3)]
        bb_max = [self.collider.to_world(axis, 0xFFFF) for axis in range(3)]
        self.write_node(0, '', bb_min, bb_max)

        with open(filename, 'w') as file:
            file.write('\n'.join(self.lines))
            file.write('\n')

def main():
    parser = argparse.ArgumentParser(description='Export kd trees from mesh collider data to obj files')
    parser.add_argument('-l', dest='leaves_only', action='store_true', help='only export leaf objects')
    parser.add_argument('file', help='file to scan for CMSH data')
    args = parser.parse_args()

    input_filename = args.file

    with open(input_filename, 'rb') as file:
        data = file.read()

    if data.startswith(b'DCA'):
        print('warning: file looks like a compressed libdragon asset, CMSH data will not be found unless it is uncompressed')

    colliders = find_mesh_colliders(data)

    if len(colliders) == 0:
        print('no mesh colliders found')
        sys.exit(1)

    base, _ = os.path.splitext(input_filename)

    for index, collider in enumerate(colliders):
        output_filename = f'{base}_{index}.obj'
        ObjWriter(collider, args.leaves_only).write(output_filename)
        print(f'wrote {output_filename} ({len(collider.triangles)} triangles, {len(collider.vertices)} vertices)')

if __name__ == '__main__':
    main()
