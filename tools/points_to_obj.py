#!/usr/bin/env python3
"""
Reads a file of points and writes them to an obj file as vertices.

Expected line format, one point per line:

    x y z

Lines that don't contain three numbers are skipped.

Usage: python3 tools/points_to_obj.py [-l] input [output]

    -l      connect consecutive points with edges
    input   file containing the points
    output  defaults to <input stem>.obj
"""

import argparse
from pathlib import Path


def parse_point(line):
    parts = line.split()

    if len(parts) != 3:
        return None

    try:
        return [float(value) for value in parts]
    except ValueError:
        return None


def main():
    parser = argparse.ArgumentParser(description='Convert a list of points to an obj file')
    parser.add_argument('-l', '--lines', action='store_true', help='connect consecutive points with edges')
    parser.add_argument('input')
    parser.add_argument('output', nargs='?')
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output) if args.output else input_path.with_suffix('.obj')

    points = []

    with open(input_path, 'r', errors='replace') as file:
        for line in file:
            point = parse_point(line)

            if point is not None:
                points.append(point)

    with open(output_path, 'w') as file:
        file.write(f'o {input_path.stem}\n')

        for point in points:
            file.write(f'v {point[0]} {point[1]} {point[2]}\n')

        if args.lines:
            for index in range(1, len(points)):
                file.write(f'l {index} {index + 1}\n')

    print(f'wrote {len(points)} points to {output_path}')


if __name__ == '__main__':
    main()
