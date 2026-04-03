import struct
import io
import typing

from . import parser
from . import variable_layout
from . import static_evaluator
from . import built_in_functions

EXPRESSION_TYPE_END = 0
EXPRESSION_TYPE_LOAD_LOCAL = 1
EXPRESSION_TYPE_LOAD_SCENE_VAR = 2
EXPRESSION_TYPE_LOAD_GLOBAL = 3
EXPRESSION_TYPE_LOAD_LITERAL = 4

EXPRESSION_TYPE_AND = 5
EXPRESSION_TYPE_OR = 6
EXPRESSION_TYPE_NOT = 7

EXPRESSION_TYPE_EQ = 8
EXPRESSION_TYPE_NEQ = 9
EXPRESSION_TYPE_GT = 10
EXPRESSION_TYPE_GTE = 11

EXPRESSION_TYPE_ADD = 12
EXPRESSION_TYPE_SUB = 13
EXPRESSION_TYPE_MUL = 14
EXPRESSION_TYPE_DIV = 15
EXPRESSION_TYPE_NEGATE = 16

EXPRESSION_TYPE_GTF = 17
EXPRESSION_TYPE_GTEF = 18

EXPRESSION_TYPE_ADDF = 19
EXPRESSION_TYPE_SUBF = 20
EXPRESSION_TYPE_MULF = 21
EXPRESSION_TYPE_DIVF = 22
EXPRESSION_TYPE_NEGATEF = 23

EXPRESSION_TYPE_ITOF = 24
EXPRESSION_TYPE_FTOI = 25

EXPRESSION_TYPE_BUILT_IN_FN = 26

EXPRESSION_TYPE_COPY = 27
EXPRESSION_TYPE_STORE = 28
EXPRESSION_TYPE_REMOVE = 29

command_to_name = {}

command_to_name[EXPRESSION_TYPE_END] = 'end'
command_to_name[EXPRESSION_TYPE_LOAD_LOCAL] = 'local'
command_to_name[EXPRESSION_TYPE_LOAD_SCENE_VAR] = 'scene'
command_to_name[EXPRESSION_TYPE_LOAD_GLOBAL] = 'global'
command_to_name[EXPRESSION_TYPE_LOAD_LITERAL] = 'literal'

command_to_name[EXPRESSION_TYPE_AND] = 'and'
command_to_name[EXPRESSION_TYPE_OR] = 'or'
command_to_name[EXPRESSION_TYPE_NOT] = 'not'

command_to_name[EXPRESSION_TYPE_EQ] = '=='
command_to_name[EXPRESSION_TYPE_NEQ] = '!='
command_to_name[EXPRESSION_TYPE_GT] = '>'
command_to_name[EXPRESSION_TYPE_GTE] = '>='

command_to_name[EXPRESSION_TYPE_ADD] = '+'
command_to_name[EXPRESSION_TYPE_SUB] = '-'
command_to_name[EXPRESSION_TYPE_MUL] = '*'
command_to_name[EXPRESSION_TYPE_DIV] = '/'
command_to_name[EXPRESSION_TYPE_NEGATE] = '-(unary)'

command_to_name[EXPRESSION_TYPE_GTF] = '>f'
command_to_name[EXPRESSION_TYPE_GTEF] = '>=f'

command_to_name[EXPRESSION_TYPE_ADDF] = '+f'
command_to_name[EXPRESSION_TYPE_SUBF] = '-f'
command_to_name[EXPRESSION_TYPE_MULF] = '*f'
command_to_name[EXPRESSION_TYPE_DIVF] = '/f'
command_to_name[EXPRESSION_TYPE_NEGATEF] = '-f(unary)'

command_to_name[EXPRESSION_TYPE_ITOF] = 'itof'
command_to_name[EXPRESSION_TYPE_FTOI] = 'foti'

command_to_name[EXPRESSION_TYPE_COPY] = 'copy'
command_to_name[EXPRESSION_TYPE_STORE] = 'store'

data_type_mapping = {
    "i8": 1,
    "i16": 2,
    "i32": 3,
    "entity_id": 2,
    "entity_spawner": 3,
    "bool": 4,
    "float": 5,
    "str": 6,
}

data_word_size_mapping = {
    "i8": 8,
    "i16": 16,
    "i32": 32,
    "entity_id": 16,
    "entity_spawner": 32,
    "bool": 1,
    "float": 32,
    "str": 8,
}

def generate_variable_address(data_type: str, bit_offset: int) -> bytes:
    if data_type.startswith('char'):
        data_type = 'str'

    return struct.pack(
        '>HH', 
        data_type_mapping[data_type], 
        bit_offset // data_word_size_mapping[data_type]
    )

# script

class ExpressionScriptLoad():
    def __init__(self, source: int, name: str, data_type: str, bit_offset: int):
        self.command: int = source
        self.name: str = name
        self.data_type: str = data_type
        self.bit_offset: int = bit_offset

    def __str__(self):
        location = command_to_name[self.command]
        return '{0} {1}: {2} 0x{3:08x}'.format(location, self.name, self.data_type, self.bit_offset)
    
    def __repr__(self):
        location = command_to_name[self.command]
        return f'<load {location} {self.name}: {self.data_type}>'

    def data_size(self) -> int:
        return 4
    
    def serialize(self, file):
        file.write(struct.pack('>B', self.command))
        file.write(generate_variable_address(self.data_type, self.bit_offset))

class ExpressionScriptIntLiteral():
    def __init__(self, value: int):
        if value == None:
            raise Exception('liternal must be an int')

        self.command: int = EXPRESSION_TYPE_LOAD_LITERAL
        self.value: int = value

    def __str__(self):
        return f'int literal {str(self.value)}'
    
    def __repr__(self):
        return f'<int literal {self.value}>'
    
    def data_size(self) -> int:
        return 4
    
    def serialize(self, file):
        file.write(struct.pack('>Bi', self.command, self.value))

class ExpressionScriptFloatLiteral():
    def __init__(self, value: float):
        self.command: int = EXPRESSION_TYPE_LOAD_LITERAL
        self.value: int = struct.unpack("<I", struct.pack("<f", value))[0]
        self.original_value = value

    def __str__(self):
        return 'float literal {0} 0x{1:08x}'.format(self.original_value, self.value)

    def __repr__(self):
        return f'<float literal {self.value}>'
    
    def data_size(self) -> int:
        return 4
    
    def serialize(self, file):
        file.write(struct.pack('>BI', self.command, self.value))

class ExpressionFunctionCall():
    def __init__(self, function_id: int, arg_count: int, result_count: int):
        self.command: int = EXPRESSION_TYPE_BUILT_IN_FN
        self.function_id = function_id
        self.arg_count: int = arg_count
        self.result_count: int = result_count
        
    def __repr__(self):
        return f'<call {self.function_id}({self.arg_count}) -> {self.result_count}>'
    
    def __str__(self):
        return f'call {self.function_id}({self.arg_count}) -> {self.result_count}'

    def data_size(self) -> int:
        return 4
    
    def serialize(self, file):
        file.write(struct.pack('>BHBB', self.command, self.function_id, self.arg_count, self.result_count))

class ExpressionCopy():
    def __init__(self, offset: int):
        self.command: int = EXPRESSION_TYPE_COPY
        self.offset: int = offset

    def __str__(self):
        return f'copy ${self.offset}'
    
    def __repr__(self):
        return f'<copy {self.offset}>'
    
    def data_size(self) -> int:
        return 1
    
    def serialize(self, file):
        file.write(struct.pack('>BB', self.command, self.offset))
        
class ExpressionStore():
    def __init__(self, offset: int):
        self.command: int = EXPRESSION_TYPE_STORE
        self.offset: int = offset

    def __str__(self):
        return f'store ${self.offset}'
    
    def __repr__(self):
        return f'<store {self.offset}>'
    
    def data_size(self) -> int:
        return 1
    
    def serialize(self, file):
        file.write(struct.pack('>BB', self.command, self.offset))
        
class ExpressionRemove():
    def __init__(self, count: int):
        self.command: int = EXPRESSION_TYPE_REMOVE
        self.count: int = count

    def __str__(self):
        return f'remove ${self.count}'
    
    def __repr__(self):
        return f'<remove {self.count}>'
    
    def data_size(self) -> int:
        return 1
    
    def serialize(self, file):
        file.write(struct.pack('>BB', self.command, self.count))
    
class ExpressionCommand():
    def __init__(self, command: int):
        self.command: int = command

    def __str__(self):
        return command_to_name[self.command]
    
    def __repr__(self):
        return f'<{command_to_name[self.command]}>'
    
    def data_size(self) -> int:
        return 0
    
    def serialize(self, file):
        file.write(struct.pack('>B', self.command))

ExpressionStep = typing.Union[ExpressionScriptLoad, ExpressionScriptIntLiteral, ExpressionScriptFloatLiteral, ExpressionFunctionCall, ExpressionCopy, ExpressionStore, ExpressionRemove, ExpressionCommand]

class ExpressionScript():
    def __init__(self, steps: list[ExpressionStep] | None = None):
        self.steps: list[ExpressionStep] = steps or []

    def __str__(self):
        return '\n'.join([str(step) for step in self.steps])
    
    def __repr__(self):
        return f"<{' '.join([repr(step) for step in self.steps])}>"
    
    def serialize(self, file):
        file.write('EXPR'.encode())

        byte_size = len(self.steps)

        for step in self.steps:
            byte_size += step.data_size()

        file.write(struct.pack('>H', byte_size + 1))

        for step in self.steps:
            step.serialize(file)
            
        # write EXPRESSION_TYPE_END
        file.write(b'\0')

    def to_bytes(self) -> bytes:
        result = io.BytesIO()
        self.serialize(result)
        return result.getvalue()

    def concat(self, other):        
        return ExpressionScript(self.steps + other.steps)
    
class ExpressionChunk():
    def __init__(self, script: ExpressionScript | None, fn_call: parser.FunctionCall):
        self.script: ExpressionScript | None = script
        self.fn_call: parser.FunctionCall = fn_call
    
def expression_concat(a: ExpressionScript | None, b: ExpressionScript | None) -> ExpressionScript | None:
    if not a:
        return b
    if not b:
        return a
    return a.concat(b) 

class ExpressionCollection():
    def __init__(self) -> None:
        self.chunks: list[ExpressionChunk] = []
        self.final_expression: ExpressionScript | None = None

    def add_step(self, step: ExpressionStep):
        if self.final_expression:
            self.final_expression.steps.append(step)
        else:
            self.final_expression = ExpressionScript([step])

    def add_call(self, call: parser.FunctionCall):
        self.chunks.append(ExpressionChunk(self.final_expression, call))
        self.final_expression = None

    def concat(self, other):
        result = ExpressionCollection()
        if not self.final_expression:
            result.chunks = self.chunks + other.chunks
            result.final_expression = other.final_expression
        elif len(other.chunks):
            result.chunks = self.chunks + [ExpressionChunk(expression_concat(self.final_expression, other.chunks[0].script), other.chunks[0].fn_call)] + other.chunks[1:]
            result.final_expression = other.final_expression
        else:
            result.chunks = self.chunks
            result.final_expression = expression_concat(self.final_expression, other.final_expression)
        return result

# types

valid_literal_types = {"int", "str", "float"}

int_operators = {'or', 'and'}
compare_operators = {'==', '!=', '>', '<', '>=', '<='}
numerical_operators = {'+', '-', '*', '/'}

type_mapping = {
    'bool': 'int',
    'i8': 'int',
    'i16': 'int',
    'i32': 'int',
    "entity_id": 'int',
    "entity_spawner": 'int',
    'entity_id': 'int',
    'entity_spawner': 'int',
    'float': 'float',
}

mirrored_operators = {
    '<' : '>',
    '<=' : '>=',
}

def is_numerical_type(type: str):
    return type == 'int' or type == 'float' or type == 'bool'

class TypeChecker():
    def __init__(self, context: variable_layout.VariableContext):
        self.expression_to_type: dict = {}
        self._context: variable_layout.VariableContext = context
        self.errors: list[str] = []

    def _report_error(self, message: str):
        self.errors.append(message)

    def _determine_type(self, expression):
        if isinstance(expression, parser.Integer):
            return 'int'
        if isinstance(expression, parser.Float):
            return 'float'
        if isinstance(expression, parser.String):
            return 'str'
        if isinstance(expression, parser.UnaryOperator):
            operand_type = self.determine_type(expression.operand)

            if operand_type == 'error':
                return 'error'

            if expression.operator.value == '-':
                if operand_type == 'int' or operand_type == 'float':
                    return operand_type
                
                self._report_error(expression.operator.format_message(f"operator does not apply to {operand_type}"))
                return 'error'
            
            if expression.operator.value == 'not':
                if operand_type == 'int':
                    return 'int'
                
                self._report_error(expression.operator.format_message(f"operator does not apply to {operand_type}"))
                return 'error'
            
            self._report_error(expression.operator.format_message('invalid operator'))
            return 'error'
        if isinstance(expression, parser.BinaryOperator):
            a_type = self.determine_type(expression.a)
            b_type = self.determine_type(expression.b)

            if a_type == 'error' or b_type == 'error':
                return 'error'
            
            if expression.operator.value in int_operators:
                if a_type != 'int' or b_type != 'int':
                    self._report_error(expression.operator.format_message(f'operator only applies to int types got {a_type} {b_type}'))
                    return 'error'
                return 'int'
            
            if expression.operator.value in compare_operators:
                if not is_numerical_type(a_type) or not is_numerical_type(b_type):
                    self._report_error(expression.operator.format_message(f'operator only applies to numerical types got {a_type} {b_type}'))
                    return 'error'
                return 'int'
            
            if expression.operator.value in numerical_operators:
                if not is_numerical_type(a_type) or not is_numerical_type(b_type):
                    self._report_error(expression.operator.format_message(f'operator only applies to numerical types got {a_type} {b_type}'))

                if a_type == 'float' or b_type == 'float':
                    return 'float'
                
                return 'int'
            
            self._report_error(expression.operator.format_message('invalid operator'))
            return 'error'
        
        if isinstance(expression, parser.Identifier):
            if expression.name.value in static_evaluator.global_constant_values:
                constant = static_evaluator.global_constant_values[expression.name.value]
                if isinstance(constant, int):
                    return 'int'
                else:
                    raise Exception('could not determine type of constant ' + expression.name.value + ' got ' + type(constant))

            type_str: str | None = self._context.get_variable_type(expression.name.value)

            if not type_str:
                self._report_error(expression.name.format_message(f'the variable {expression.name.value} is not defined'))
                return 'error'
            
            if type_str in type_mapping:
                return type_mapping[type_str]
            
            if type_str.startswith('char['):
                return 'str'
            
            self._report_error(expression.name.format_message(f'unknown type {type_str}'))
            return 'error'
        
        if isinstance(expression, parser.FunctionCall):
            index, fn = self._context.lookup_function(expression.name.value)

            if not fn:
                built_in = built_in_functions.lookup(expression.name.value)

                if built_in:
                    fn = built_in.definition

            if fn:
                if len(expression.args) != len(fn.args):
                    self._report_error(expression.name.format_message(f"expected {len(fn.args)} arguments got {len(expression.args)}"))

                for i in range(0, min(len(expression.args), len(fn.args))):
                    element_type = self.determine_type(expression.args[i])

                    if element_type != type_mapping[fn.args[i].type_name.name.value]:
                        self._report_error(expression.args[i].at.format_message(f"expected {fn.args[i].type_name.name.value} got {element_type}"))
                
                if len(fn.return_types) > 0:
                    return type_mapping[fn.return_types[0].name.value]
                else:
                    self._report_error(expression.name.format_message(f"function returns void expected a value"))
                    return 'error'
            
            self._report_error(expression.name.format_message(f"could not find function name {expression.name.value}"))
            return 'error'
    
        raise Exception('unknown expression type')

    def determine_type(self, expression) -> str:
        result = self._determine_type(expression)
        self.expression_to_type[expression] = result
        return result
    
    
def type_from_pytype(pytype):
    if isinstance(pytype, int):
        return 'int'
    if isinstance(pytype, float):
        return 'float'
    if isinstance(pytype, str):
        return 'str'
    
    return 'error'
    
class ExpressionGenerator():
    def __init__(self, context: variable_layout.VariableContext, type_info: TypeChecker, eval: static_evaluator.StaticEvaluator):
        self.context: variable_layout.VariableContext = context
        self.type_info: TypeChecker = type_info
        self.static_evaluator: static_evaluator.StaticEvaluator = eval

    def generate_to_type(self, expression: parser.Expression, to_type: str, script: ExpressionCollection, retc: int):
        self.generate(expression, script, retc)
        
        from_type = self.type_info.expression_to_type[expression]

        if from_type == to_type:
            return
        
        if from_type == 'int' and to_type == 'float':
            script.add_step(ExpressionCommand(EXPRESSION_TYPE_ITOF))
        
        elif from_type == 'float' and to_type == 'int':
            script.add_step(ExpressionCommand(EXPRESSION_TYPE_FTOI))

        else:
            raise Exception(f'Connot convert from {from_type} to {to_type}')
    
    def generate_identifier_read(self, expression: parser.Identifier):
        name = expression.name.value

        local_offset = self.context.get_fn_local_offset(name)

        if local_offset != None:
            return ExpressionCopy(local_offset)
        
        source = EXPRESSION_TYPE_LOAD_GLOBAL

        if self.context.is_global(name):
            source = EXPRESSION_TYPE_LOAD_GLOBAL
        elif self.context.is_scene_var(name):
            source = EXPRESSION_TYPE_LOAD_SCENE_VAR
        else:
            raise Exception(expression.name.format_message('Variable not found'))

        data_type = self.context.get_variable_type(name)

        if not data_type:
            raise Exception(expression.name.format_message('Variable not found'))
        
        offset = self.context.get_variable_offset(name)

        return ExpressionScriptLoad(source, name, data_type, offset)

    def generate(self, expression: parser.Expression, script: ExpressionCollection, retc: int):
        if not expression in self.static_evaluator.literal_value:
            raise Exception(expression.at.format_message('expression is missing a type'))

        literal_value = self.static_evaluator.literal_value[expression]

        if not literal_value is None:
            if type_from_pytype(literal_value) != self.type_info.expression_to_type[expression]:
                raise Exception(f"mismatched type {type_from_pytype(literal_value)} {self.type_info.expression_to_type[expression]}")
            if type_from_pytype(literal_value) == 'int':
                script.add_step(ExpressionScriptIntLiteral(literal_value))
                self.context.modify_stack_size(1)
            elif type_from_pytype(literal_value) == 'float':
                script.add_step(ExpressionScriptFloatLiteral(literal_value))
                self.context.modify_stack_size(1)
            else:
                raise Exception(f"unknown literal type {type_from_pytype(literal_value)}")
            return
        
        if isinstance(expression, parser.UnaryOperator):
            self.generate(expression.operand, script, 1)
            if expression.operator.value == '-':
                if self.type_info.expression_to_type[expression] == 'int':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_NEGATE))
                else:
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_NEGATEF))

            if expression.operator.value == 'not':
                script.add_step(ExpressionCommand(EXPRESSION_TYPE_NOT))

        if isinstance(expression, parser.BinaryOperator):
            operator = expression.operator.value

            cast_to_type = self.type_info.expression_to_type[expression]
            a_type = self.type_info.expression_to_type[expression.a]
            b_type = self.type_info.expression_to_type[expression.a]

            if operator in compare_operators and (a_type == 'float' or b_type == 'float'):
                cast_to_type = 'float'


            if operator in mirrored_operators:
                operator = mirrored_operators[operator]
                self.generate_to_type(expression.a, cast_to_type, script, 1)
                self.generate_to_type(expression.b, cast_to_type, script, 1)
            else:
                self.generate_to_type(expression.b, cast_to_type, script, 1)
                self.generate_to_type(expression.a, cast_to_type, script, 1)

            self.context.modify_stack_size(-1)

            if operator == 'and':
                script.add_step(ExpressionCommand(EXPRESSION_TYPE_AND))
            if operator == 'or':
                script.add_step(ExpressionCommand(EXPRESSION_TYPE_OR))

            if operator == '==':
                script.add_step(ExpressionCommand(EXPRESSION_TYPE_EQ))
            if operator == '!=':
                script.add_step(ExpressionCommand(EXPRESSION_TYPE_NEQ))

            if cast_to_type == 'int':
                if operator == '>':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_GT))
                if operator == '>=':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_GTE))
                    
                if operator == '+':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_ADD))
                if operator == '-':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_SUB))
                if operator == '*':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_MUL))
                if operator == '/':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_DIV))
            else:
                if operator == '>':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_GTF))
                if operator == '>=':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_GTEF))

                if operator == '+':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_ADDF))
                if operator == '-':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_SUBF))
                if operator == '*':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_MULF))
                if operator == '/':
                    script.add_step(ExpressionCommand(EXPRESSION_TYPE_DIVF))

        if isinstance(expression, parser.Identifier):
            script.add_step(self.generate_identifier_read(expression))
            self.context.modify_stack_size(1)

        if isinstance(expression, parser.FunctionCall):
            _index, fn = self.context.lookup_function(expression.name.value)

            if fn:
                for arg in expression.args:
                    self.generate(arg, script, 1)

                script.add_call(expression)
                self.context.modify_stack_size(retc - len(expression.args))
                return

            built_in = built_in_functions.lookup(expression.name.value)

            if not built_in:
                raise Exception(expression.name.format_message(f"Could not find function {expression.name.value}"))
            
            for i, arg in enumerate(expression.args):
                self.generate(arg, script, 1)

            script.add_step(ExpressionFunctionCall(built_in.index, len(expression.args), 1))
            self.context.modify_stack_size(1 - len(expression.args))

def generate_script(expression, context: variable_layout.VariableContext, expected_type: str | None = None, retc: int = 1) -> ExpressionCollection:
    type_info = TypeChecker(context)
    actual_type = type_info.determine_type(expression)

    if len(type_info.errors):
        print('\n\n'.join(type_info.errors))
        return ExpressionCollection()
    
    eval = static_evaluator.StaticEvaluator()
    eval.check_for_literals(expression)

    generator = ExpressionGenerator(context, type_info, eval)

    result = ExpressionCollection()
    
    if expected_type != None and actual_type != expected_type:
        generator.generate_to_type(expression, expected_type, result, retc)
    else:
        generator.generate(expression, result, retc)

    return result