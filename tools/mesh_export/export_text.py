import sys
import os

sys.path.append(os.path.dirname(__file__))

import cutscene.parser

script_start_index = sys.argv.index('--')

output = open(sys.argv[script_start_index+1], "w")

def traverse_statements(statements: list):
    for statement in statements:
        if isinstance(statement, cutscene.parser.CutsceneStep):
            if statement.name.value == "say" or statement.name.value == "ask":
                output.write(str(statement.parameters[0]))
                output.write('\n')
        elif isinstance(statement, cutscene.parser.IfStatement):
            traverse_statements(statement.statements)

            if statement.else_block:
                traverse_statements(statement.else_block)


for script_filename in sys.argv[script_start_index+2:]:
    print(f"processing {script_filename}")

    with open(script_filename, "r", encoding="utf-8") as file:
        content = file.read()

    parsed = cutscene.parser.parse(content, script_filename)

    output.write(f"for script {script_filename}\n")
    traverse_statements(parsed.statements)

    for fn in parsed.functions:
        traverse_statements(fn.body)


output.close()