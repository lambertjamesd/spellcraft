#!/usr/bin/env python3
"""
Reads a file of quaternions and writes an obj file containing a copy of an
arrow mesh rotated by each quaternion.

Expected line format, one quaternion per line:

    x y z w

Lines that don't contain four numbers are skipped. Quaternions are normalized
before being applied.

The mesh is a large primary arrow with a smaller secondary arrow so the full
orientation is visible. Each copy is written as its own object named
quat_<index> where index is the line order of the valid quaternions.

Usage: python3 tools/quats_to_obj.py [options] input [output]

    -p, --primary AXIS      direction of the primary arrow, default +z
    -s, --secondary AXIS    direction of the secondary arrow, default +y
    --spacing DISTANCE      distance between copies along x, default 3
                            use 0 to place every copy at the origin
    input                   file containing the quaternions
    output                  defaults to <input stem>.obj

AXIS is one of x, y, z optionally prefixed with + or -
"""

import argparse
import math
from pathlib import Path

PRIMARY_LENGTH = 1.0
SECONDARY_LENGTH = 0.5

AXES = {
    'x': [1.0, 0.0, 0.0],
    'y': [0.0, 1.0, 0.0],
    'z': [0.0, 0.0, 1.0],
}


def parse_axis(value):
    sign = 1.0

    if value[:1] in ('+', '-'):
        sign = -1.0 if value[0] == '-' else 1.0
        value = value[1:]

    axis = AXES.get(value.lower())

    if axis is None:
        raise argparse.ArgumentTypeError(f'invalid axis {value!r}, expected x, y, or z with optional +/-')

    return [component * sign for component in axis]


def parse_quat(line):
    parts = line.split()

    if len(parts) != 4:
        return None

    try:
        x, y, z, w = [float(value) for value in parts]
    except ValueError:
        return None

    length = math.sqrt(x * x + y * y + z * z + w * w)

    if length == 0:
        return None

    return [x / length, y / length, z / length, w / length]


def cross(a, b):
    return [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]


def rotate(quat, point):
    # v' = v + 2w(q x v) + 2 q x (q x v)
    q = quat[0:3]
    w = quat[3]
    t = [component * 2 for component in cross(q, point)]
    u = cross(q, t)

    return [point[i] + w * t[i] + u[i] for i in range(3)]


def build_arrow(direction, length):
    """
    Builds an arrow from the origin along direction with a square shaft and a
    pyramid head. Returns (vertices, faces) with faces as 0 based indices.
    """
    # pick a perpendicular basis so that u x v = direction
    helper = [1.0, 0.0, 0.0] if abs(direction[0]) < 0.9 else [0.0, 1.0, 0.0]
    u = cross(helper, direction)
    u_length = math.sqrt(sum(component * component for component in u))
    u = [component / u_length for component in u]
    v = cross(direction, u)

    def point(a, b, c):
        return [u[i] * a + v[i] * b + direction[i] * c for i in range(3)]

    shaft_width = length * 0.04
    head_width = length * 0.12
    shaft_length = length * 0.7

    # corners counter clockwise around direction
    corners = [(1, 1), (-1, 1), (-1, -1), (1, -1)]

    vertices = []
    vertices += [point(a * shaft_width, b * shaft_width, 0) for a, b in corners]
    vertices += [point(a * shaft_width, b * shaft_width, shaft_length) for a, b in corners]
    vertices += [point(a * head_width, b * head_width, shaft_length) for a, b in corners]
    vertices.append(point(0, 0, length))

    shaft_bottom = 0
    shaft_top = 4
    head_base = 8
    tip = 12

    faces = []
    faces.append([shaft_bottom + 3, shaft_bottom + 2, shaft_bottom + 1, shaft_bottom + 0])
    faces.append([head_base + 3, head_base + 2, head_base + 1, head_base + 0])

    for i in range(4):
        next_i = (i + 1) % 4
        faces.append([shaft_bottom + i, shaft_bottom + next_i, shaft_top + next_i, shaft_top + i])
        faces.append([head_base + i, head_base + next_i, tip])

    return vertices, faces


def build_mesh(primary, secondary):
    vertices, faces = build_arrow(primary, PRIMARY_LENGTH)
    secondary_vertices, secondary_faces = build_arrow(secondary, SECONDARY_LENGTH)

    offset = len(vertices)
    vertices += secondary_vertices
    faces += [[index + offset for index in face] for face in secondary_faces]

    return vertices, faces


def main():
    parser = argparse.ArgumentParser(description='Convert a list of quaternions to an obj file of rotated arrows')
    parser.add_argument('-p', '--primary', type=parse_axis, default='+z', help='direction of the primary arrow, default +z')
    parser.add_argument('-s', '--secondary', type=parse_axis, default='+y', help='direction of the secondary arrow, default +y')
    parser.add_argument('--spacing', type=float, default=3.0, help='distance between copies along x, default 3')
    parser.add_argument('input')
    parser.add_argument('output', nargs='?')
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output) if args.output else input_path.with_suffix('.obj')

    quats = []

    with open(input_path, 'r', errors='replace') as file:
        for line in file:
            quat = parse_quat(line)

            if quat is not None:
                quats.append(quat)

    vertices, faces = build_mesh(args.primary, args.secondary)

    with open(output_path, 'w') as file:
        for index, quat in enumerate(quats):
            file.write(f'o quat_{index}\n')

            base = index * len(vertices)
            offset_x = index * args.spacing

            for vertex in vertices:
                rotated = rotate(quat, vertex)
                file.write(f'v {rotated[0] + offset_x} {rotated[1]} {rotated[2]}\n')

            for face in faces:
                file.write('f ' + ' '.join(str(base + i + 1) for i in face) + '\n')

    print(f'wrote {len(quats)} orientations to {output_path}')


if __name__ == '__main__':
    main()
