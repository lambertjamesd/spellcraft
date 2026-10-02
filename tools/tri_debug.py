#!/usr/bin/env python3
"""
Scans a debug log for triangle lines and writes them to an obj file.

Expected line format:

    tri x y z, x y z, x y z

Usage: python3 tools/tri_debug.py [input] [output]

    input   defaults to out.txt
    output  defaults to <input stem>.obj
"""

import re
import sys
from pathlib import Path

NUMBER = r'[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?'
VERTEX = rf'({NUMBER})\s+({NUMBER})\s+({NUMBER})'
TRI_PATTERN = re.compile(rf'tri\s+{VERTEX}\s*,\s*{VERTEX}\s*,\s*{VERTEX}')


def main():
    input_path = Path(sys.argv[1] if len(sys.argv) > 1 else 'out.txt')
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else input_path.with_suffix('.obj')

    tris = []

    with open(input_path, 'r', errors='replace') as file:
        for line in file:
            match = TRI_PATTERN.search(line)

            if not match:
                continue

            values = [float(value) for value in match.groups()]
            tris.append([values[0:3], values[3:6], values[6:9]])

    with open(output_path, 'w') as file:
        file.write('o tris\n')

        for index, tri in enumerate(tris):
            for vertex in tri:
                file.write(f'v {vertex[0]} {vertex[1]} {vertex[2]}\n')

            base = index * 3
            file.write(f'f {base + 1} {base + 2} {base + 3}\n')

    print(f'wrote {len(tris)} triangles to {output_path}')


if __name__ == '__main__':
    main()
