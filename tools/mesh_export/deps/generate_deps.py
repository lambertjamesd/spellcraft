import os

# output_filename should be relative to the project root. If root is given
# the dependency file is written relative to it instead of the working directory
def generate_deps(output_filename, script_path, root=None):
    dep_output = output_filename.replace('filesystem/', 'build/assets/')
    rule_output = output_filename.replace('build/assets/', 'filesystem/')

    if root:
        dep_output = os.path.join(root, dep_output)

    with open(f"{os.path.splitext(script_path)[0]}.d.template", "r") as input:
        input_contents = input.read()

    with open(f'{os.path.splitext(dep_output)[0]}.d', "w") as output:
        # output.write(input_contents.replace('FILENAME', rule_output))
        output.write(f"{rule_output}:")
