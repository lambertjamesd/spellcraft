import sys
import argparse
import os
import re
import json

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.cutscene.parser

func_pattern = r"// func .*\n"

def convert_data_type_to_json(data_type: mesh_export.cutscene.parser.DataType):
    return {
        "name": data_type.name.value,
        "count": data_type.count,
    }

def convert_def_to_json(definition: mesh_export.cutscene.parser.FunctionDefinition):
    return {
        "name": definition.name.value,
        "args": [{
            "name": arg.name.value,
            "type_name": convert_data_type_to_json(arg.type_name),
        } for arg in definition.args],
        "return_types": [convert_data_type_to_json(ret) for ret in definition.return_types],
    }

def read_defs_in_file(filename: str):
    with open(filename) as file:
        file_source = file.read()

    curr_line = 1
    curr_col =  1

    last_index = 0

    defs = []
    
    did_fail = False

    for match in re.finditer(func_pattern, file_source):
        start = match.start()+2

        while last_index < start:
            if file_source[last_index] == '\n':
                curr_line += 1
                curr_col = 1
            else:
                curr_col += 1

            last_index += 1

        try:
            function_def = mesh_export.cutscene.parser.parse_function_definition(
                file_source[start:match.end()] + ' end', 
                filename,
                start_line=curr_line,
                start_col=curr_col
            )
            defs.append(convert_def_to_json(function_def))
        except mesh_export.cutscene.parser.ParseError as err:
            did_fail = True
            print(err)

    return defs, did_fail

if __name__ == "__main__":
    step_defs, did_step_fail = read_defs_in_file('src/cutscene/cutscene_step_fn.c')
    expr_defs, did_expr_fail = read_defs_in_file('src/cutscene/expression_fn.c')

    if did_step_fail or did_expr_fail:
        sys.exit(1)

    with open('build/cutscene/function_defs.json', 'w') as file:
        json.dump({
            "step_defs": step_defs,
            "expr_defs": expr_defs,
        }, file, indent=4)