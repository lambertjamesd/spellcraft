import sys
import argparse
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.cutscene.parser
import mesh_export.cutscene.variable_layout
import mesh_export.cutscene.step_generator

from mesh_export.deps import generate_deps

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('-g', '--globals')
    parser.add_argument('-c', '--cutscenes', required=False)

    parser.add_argument('input')
    parser.add_argument('output')

    args = parser.parse_args()
    result: mesh_export.cutscene.parser.Cutscene | None = None

    globals = mesh_export.cutscene.variable_layout.VariableLayout()
    scene_vars = mesh_export.cutscene.variable_layout.VariableLayout()

    with open(args.globals) as file:
        globals.deserialize(file)

    if args.cutscenes:
        with open(args.cutscenes) as file:
            scene_vars.deserialize(file)

    generate_deps.generate_deps(args.output, os.path.relpath(__file__))
    
    try:
        with open(args.input) as file:
            result = mesh_export.cutscene.parser.parse(file.read(), args.input)

        local_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()

        for local_var in result.locals:
            local_builder.add_variable(local_var)

        locals = local_builder.build()

        context = mesh_export.cutscene.variable_layout.VariableContext(globals, scene_vars, locals)

        errors: list[str] = []
        mesh_export.cutscene.step_generator.validate_steps(result.statements, errors, context)

        if len(errors) > 0:
            print('\n\n'.join(errors))
            sys.exit(1)

        with open(args.output, 'wb') as file:
            mesh_export.cutscene.step_generator.generate_steps(file, result, context)
    except Exception as e:
        print(e)
        raise e