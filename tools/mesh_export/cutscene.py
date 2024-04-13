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
    
    with open(args.input) as file:
        result = cutscene.parser.parse(file.read(), args.input)

    local_builder = cutscene.variable_layout.VariableLayoutBuilder()

    for local_var in result.locals:
        local_builder.add_variable(local_var)

    locals = local_builder.build()

    context = cutscene.variable_layout.VariableContext(globals, locals)

    errors: list[str] = []
    cutscene.step_generator.validate_steps(result.statements, errors, context)

    if len(errors) > 0:
        print('\n\n'.join(errors))
        sys.exit(1)

    print(result)