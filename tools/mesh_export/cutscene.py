import sys
import argparse
import cutscene.tokenizer
import cutscene.parser
import cutscene.variable_layout
import cutscene.step_generator

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('-g', '--globals')

    parser.add_argument('input')
    parser.add_argument('output')

    args = parser.parse_args()
    result: cutscene.parser.Cutscene = None

    globals = cutscene.variable_layout.VariableLayout()

    with open(args.globals) as file:
        globals.deserialize(file)
    
    try:
        with open(args.input) as file:
            result = cutscene.parser.parse(file.read(), args.input)

        local_builder = cutscene.variable_layout.VariableLayoutBuilder()

        for local_var in result.locals:
            local_builder.add_variable(local_var)

        locals = local_builder.build()

        scene_builder = cutscene.variable_layout.VariableLayoutBuilder()

        for scene_var in result.scene_vars:
            scene_builder.add_variable(scene_var)

        scene_vars = scene_builder.build()

        context = cutscene.variable_layout.VariableContext(globals, scene_vars, locals)

        errors: list[str] = []
        cutscene.step_generator.validate_steps(result.statements, errors, context)

        if len(errors) > 0:
            print('\n\n'.join(errors))
            sys.exit(1)

        with open(args.output, 'wb') as file:
            cutscene.step_generator.generate_steps(file, result.statements, context)
    except:
        sys.exit(1)