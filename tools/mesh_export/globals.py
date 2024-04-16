import sys
import argparse
import cutscene.tokenizer
import cutscene.parser
import cutscene.variable_layout

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('output')
    parser.add_argument('input', nargs='*')

    args = parser.parse_args()
    result = cutscene.variable_layout.VariableLayoutBuilder()

    success = True
    
    for input in args.input:
        with open(input) as file:
            current_cutscene = cutscene.parser.parse(file.read(), args.input)

            for global_var in current_cutscene.globals:
                success = result.add_variable(global_var) and success

    if not success:
        sys.exit(1)

    with open(args.output + '.dat', 'wb') as file:
        result.build().write_default_values(file)

    with open(args.output + '.json', 'w') as file:
        result.serialize(file)

    