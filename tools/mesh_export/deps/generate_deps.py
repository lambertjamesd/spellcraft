import os

def _escape_make_path(path: str) -> str:
    return path.replace('\\', '/').replace(' ', '\\ ').replace('#', '\\#').replace('$', '$$')

# output_filename should be relative to the project root. If root is given
# the dependency file is written relative to it instead of the working directory
# dependencies is an optional list of extra files, relative to the project root,
# that should cause output_filename to be rebuilt when they change
def generate_deps(output_filename, script_path, root=None, dependencies=None):
    dep_output = output_filename.replace('filesystem/', 'build/assets/')
    rule_output = output_filename.replace('build/assets/', 'filesystem/')

    if root:
        dep_output = os.path.join(root, dep_output)

    with open(f"{os.path.splitext(script_path)[0]}.d.template", "r") as input:
        input_contents = input.read()

    with open(f'{os.path.splitext(dep_output)[0]}.d', "w") as output:
        # output.write(input_contents.replace('FILENAME', rule_output))
        dependencies = [_escape_make_path(dep) for dep in (dependencies or [])]

        output.write(f"{rule_output}:")

        for dep in dependencies:
            output.write(f" \\\n    {dep}")

        output.write("\n")

        # empty rules for each dependency so make doesn't fail if one is deleted
        for dep in dependencies:
            output.write(f"\n{dep}:\n")
