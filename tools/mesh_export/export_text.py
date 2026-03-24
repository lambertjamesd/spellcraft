import sys
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.cutscene.parser

script_start_index = sys.argv.index('--')

output = open(sys.argv[script_start_index+1], "w")

def traverse_statements(statements: list):
    for statement in statements:
        if isinstance(statement, mesh_export.cutscene.parser.CutsceneStep):
            if statement.name.value == "say" or statement.name.value == "ask":
                output.write(str(statement.parameters[0]))
                output.write('\n')
        elif isinstance(statement, mesh_export.cutscene.parser.IfStatement):
            traverse_statements(statement.statements)

            if statement.else_block:
                traverse_statements(statement.else_block)


for script_filename in sys.argv[script_start_index+2:]:
    print(f"processing {script_filename}")

    with open(script_filename, "r", encoding="utf-8") as file:
        content = file.read()

    parsed = mesh_export.cutscene.parser.parse(content, script_filename)

    output.write(f"for script {script_filename}\n")
    traverse_statements(parsed.statements)

    for fn in parsed.functions:
        traverse_statements(fn.body)


output.close()