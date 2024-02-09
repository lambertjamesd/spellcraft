import argparse
import material_writer.material
import material_writer.serialize

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
        default_material = material_writer.material.parse_material(args.default)

    print(f'Writing material to {args.output}')
    result = material_writer.material.parse_material(args.input)
    print(result)
    material_writer.serialize.serialize_material(args.output, result)
