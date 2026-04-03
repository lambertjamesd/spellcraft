import json

from . import parser
from . import tokenizer

EXPRESSION_BUILT_IN_ARE_TOUCHING = 0
EXPRESSION_BUILT_ASK_RESPONSE = 1
EXPRESSION_BUILT_STOPWATCH_TIME = 2

class FunctionParameter():
    def __init__(self, name: str, type: str):
        self.name: str = name
        self.type: str = type

class BuiltInFunction():
    def __init__(self, index: int, return_type: str, args: list[FunctionParameter]):
        self.index: int = index
        self.return_type: str = return_type
        self.args: list[FunctionParameter] = args

    def get_arg_type(self, index: int) -> str:
        if index < len(self.args):
            return self.args[index].type

        return 'error'


_built_in_functions = {
    "are_touching": BuiltInFunction(EXPRESSION_BUILT_IN_ARE_TOUCHING, "int", [FunctionParameter("a", "int"), FunctionParameter("b", "int")]),
    "ask_response": BuiltInFunction(EXPRESSION_BUILT_ASK_RESPONSE, "int", []),
    "stopwatch_time": BuiltInFunction(EXPRESSION_BUILT_STOPWATCH_TIME, "float", []),
}

def lookup(name: str) -> BuiltInFunction | None:
    if name in _built_in_functions:
        return _built_in_functions[name]
    
    return None
    
blocking_functions: dict[str, tuple[int, parser.FunctionDefinition]] = {}
non_blocking_functions: dict[str, tuple[int, parser.FunctionDefinition]] = {}

def build_fake_token(value) -> tokenizer.Token:
    return tokenizer.Token('identifier', value, 0, None)

def build_data_type(type_json) -> parser.DataType:
    return parser.DataType(build_fake_token(type_json['name']), type_json['count'])

def build_def_arg(type_json) -> parser.FunctionDefinitionArg:
    return parser.FunctionDefinitionArg(build_fake_token(type_json['name']), build_data_type(type_json['type_name']))

def build_function_def(definition_json) -> parser.FunctionDefinition:
    return parser.FunctionDefinition(
        build_fake_token('func'),
        build_fake_token(definition_json['name']),
        [build_def_arg(arg) for arg in definition_json['args']],
        [build_data_type(ret) for ret in definition_json['return_types']],
        [],
        built_in=True
    )

try:
    with open('build/cutscene/function_defs.json', 'r') as file:
        file_contents = json.load(file)

        for index, definition_json in enumerate(file_contents['step_defs']):
            definition = build_function_def(definition_json)
            blocking_functions[definition.name.value] = (index, definition)

        for index, definition_json in enumerate(file_contents['expr_defs']):
            definition = build_function_def(definition_json)
            non_blocking_functions[definition.name.value] = (index, definition)
except Exception as e:
    print(f'warning: failed to load function_defs {e}')
