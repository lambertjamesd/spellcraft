import argparse
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.material
import mesh_export.entities.serialize

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('input')
    parser.add_argument('output')
    parser.add_argument('-d', '--default')

    args = parser.parse_args()

    default_material = None

    if args.default != args.input and args.default:
        default_material = mesh_export.entities.material.parse_material(args.default)

    print(f'Writing material to {args.output}')
    result = mesh_export.entities.material.parse_material(args.input)
    print(result)
    mesh_export.entities.serialize.serialize_material(args.output, result)
