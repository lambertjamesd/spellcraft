import os

def generate_deps(output_filename, script_path):
    dep_output = output_filename.replace('filesystem/', 'build/assets/')
    rule_output = output_filename.replace('build/assets/', 'filesystem/')

    with open(f"{os.path.splitext(script_path)[0]}.d.template", "r") as input:
        input_contents = input.read()

    with open(f'{os.path.splitext(dep_output)[0]}.d', "w") as output:
        output.write(input_contents.replace('FILENAME', rule_output))
