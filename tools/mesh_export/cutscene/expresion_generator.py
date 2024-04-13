import struct
import sys

from . import parser
from . import variable_layout

EXPRESSION_TYPE_END = 0
EXPRESSION_TYPE_LOAD_LOCAL = 1
EXPRESSION_TYPE_LOAD_GLOBAL = 2
EXPRESSION_TYPE_LOAD_LITERAL = 3

EXPRESSION_TYPE_AND = 4
EXPRESSION_TYPE_OR = 5
EXPRESSION_TYPE_NOT = 6

EXPRESSION_TYPE_EQ = 7
EXPRESSION_TYPE_NEQ = 8
EXPRESSION_TYPE_GT = 9
EXPRESSION_TYPE_GTE = 10

EXPRESSION_TYPE_ADD = 11
EXPRESSION_TYPE_SUB = 12
EXPRESSION_TYPE_MUL = 13
EXPRESSION_TYPE_DIV = 14
EXPRESSION_TYPE_NEGATE = 15

EXPRESSION_TYPE_GTF = 16
EXPRESSION_TYPE_GTEF = 17

EXPRESSION_TYPE_ADDF = 18
EXPRESSION_TYPE_SUBF = 19
EXPRESSION_TYPE_MULF = 20
EXPRESSION_TYPE_DIVF = 21
EXPRESSION_TYPE_NEGATEF = 22

EXPRESSION_TYPE_ITOF = 23
EXPRESSION_TYPE_FTOI = 24

command_to_name = {}

command_to_name[EXPRESSION_TYPE_END] = 'end'
command_to_name[EXPRESSION_TYPE_LOAD_LOCAL] = 'local'
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

data_type_mapping = {
    "i8": 1,
    "i16": 2,
    "i32": 3,
    "bool": 4,
    "float": 5,
}

data_word_size_mapping = {
    "i8": 8,
    "i16": 16,
    "i32": 32,
    "bool": 1,
    "float": 32,
}

# script

class ExpresionScriptLoad():
    def __init__(self, is_global: bool, name: str, data_type: str, bit_offset: int):
        self.command: int = EXPRESSION_TYPE_LOAD_LOCAL
        if is_global:
            self.command = EXPRESSION_TYPE_LOAD_GLOBAL
        self.name: str = name
        self.data_type: str = data_type
        self.bit_offset: int = bit_offset

    def __str__(self):
        location = 'local' if self.command == EXPRESSION_TYPE_LOAD_LOCAL else 'global'
        return '{0} {1}: {2} 0x{3:08x}'.format(location, self.name, self.data_type, self.bit_offset)
    
    def has_data() -> bool:
        return True
    
    def serialize(self, file):
        file.write(struct.pack(
            '>BHH', 
            self.command, 
            data_type_mapping[self.data_type], 
            self.bit_offset // data_word_size_mapping[self.data_type]
        ))

class ExpresionScriptIntLiteral():
    def __init__(self, value: int):
        self.command: int = EXPRESSION_TYPE_LOAD_LITERAL
        self.value: int = value

    def __str__(self):
        return f'int literal {str(self.value)}'
    
    def has_data() -> bool:
        return True
    
    def serialize(self, file):
        file.write(struct.pack('>Bi', self.command, self.value))

class ExpresionScriptFloatLiteral():
    def __init__(self, value: float):
        self.command: int = EXPRESSION_TYPE_LOAD_LITERAL
        self.value: int = struct.unpack("<I", struct.pack("<f", value))[0]
        self.original_value = value

    def __str__(self):
        return 'float literal {0} 0x{1:08x}'.format(self.original_value, self.value)

    def has_data() -> bool:
        return True
    
    def serialize(self, file):
        file.write(struct.pack('>BI', self.command, self.value))
    
class ExpressionCommand():
    def __init__(self, command: int):
        self.command: int = command

    def __str__(self):
        return command_to_name[self.command]
    
    def has_data() -> bool:
        return False
    
    def serialize(self, file):
        file.write(struct.pack('>B', self.command))

class ExpressionScript():
    def __init__(self):
        self.steps: list = []

    def __str__(self):
        return '\n'.join([str(step) for step in self.steps])
    
    def serialize(self, file):
        file.write('EXPR'.encode())

        byte_size = len(self.steps)

        for step in self.steps:
            if step.has_data():
                byte_size += 4

        file.write(struct.pack('>H', byte_size))

        for step in self.steps:
            step.serialize(file)

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
    'float': 'float',
}

global_constant_types = {
    'true': 'int',
    'false': 'int',
}

global_constant_values = {
    'true': 1,
    'false': 0,
}

mirrored_operators = {
    '<' : '>',
    '<=' : '>=',
}

def is_numerical_type(type: str):
    return type == 'int' or type == 'float'

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
            if expression.name.value in global_constant_types:
                return global_constant_types[expression.name.value]

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
    
        raise Exception('unknown expression type')

    def determine_type(self, expression):
        result = self._determine_type(expression)
        self.expression_to_type[expression] = result
        return result
    
class StaticEvaluator():
    def __init__(self):
        self.literal_value = {}

    def _check_for_literals(self, expression):
        if isinstance(expression, parser.Integer):
            return int(expression.value.value)
        if isinstance(expression, parser.Float):
            return float(expression.value.value)
        if isinstance(expression, parser.String):
            return None
        if isinstance(expression, parser.UnaryOperator):
            operand_value = self.check_for_literals(expression.operand)

            if operand_value is None:
                return None

            if expression.operator.value == '-':
                return -operand_value

            if expression.operator.value == 'not':
                return 0 if operand_value else 1
        if isinstance(expression, parser.BinaryOperator):
            a_value = self.check_for_literals(expression.a)
            b_value = self.check_for_literals(expression.b)

            if a_value is None or b_value is None:
                return None
            
            if expression.operator.value == 'and':
                return 1 if a_value and b_value else 0
            if expression.operator.value == 'or':
                return 1 if a_value or b_value else 0
            if expression.operator.value == '==':
                return 1 if a_value == b_value else 0
            if expression.operator.value == '!=':
                return 1 if a_value != b_value else 0
            if expression.operator.value == '>':
                return 1 if a_value > b_value else 0
            if expression.operator.value == '<':
                return 1 if a_value < b_value else 0
            if expression.operator.value == '>=':
                return 1 if a_value >= b_value else 0
            if expression.operator.value == '<=':
                return 1 if a_value <= b_value else 0
            if expression.operator.value == '+':
                return a_value + b_value
            if expression.operator.value == '-':
                return a_value - b_value
            if expression.operator.value == '*':
                return a_value * b_value
            if expression.operator.value == '/':
                if type_from_pytype(a_value) == 'int' and type_from_pytype(b_value) == 'int':
                    return a_value // b_value
                return a_value / b_value
        if isinstance(expression, parser.Identifier):
            if expression.name.value in global_constant_values:
                return global_constant_values[expression.name.value]
        
        return None

    def check_for_literals(self, expression):
        result = self._check_for_literals(expression)
        self.literal_value[expression] = result
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
    def __init__(self, context: variable_layout.VariableContext, type_info: TypeChecker, static_evaluator: StaticEvaluator):
        self.context: variable_layout.VariableContext = context
        self.type_info: TypeChecker = type_info
        self.static_evaluator: StaticEvaluator = static_evaluator

    def generate_to_type(self, expression, to_type: str, script: ExpressionScript):
        self.generate(expression, script)
        
        from_type = self.type_info.expression_to_type[expression]

        if from_type == to_type:
            return
        
        if from_type == 'int' and to_type == 'float':
            script.steps.append(ExpressionCommand(EXPRESSION_TYPE_ITOF))
        
        elif from_type == 'float' and to_type == 'int':
            script.steps.append(ExpressionCommand(EXPRESSION_TYPE_FTOI))

        else:
            raise Exception(f'Connot convert from {from_type} to {to_type}')
            

    def generate(self, expression, script: ExpressionScript):
        literal_value = self.static_evaluator.literal_value[expression]

        if not literal_value is None:
            if type_from_pytype(literal_value) != self.type_info.expression_to_type[expression]:
                raise Exception(f"mismatched type {type_from_pytype(literal_value)} {self.type_info.expression_to_type[expression]}")
            if type_from_pytype(literal_value) == 'int':
                script.steps.append(ExpresionScriptIntLiteral(literal_value))
            elif type_from_pytype(literal_value) == 'float':
                script.steps.append(ExpresionScriptFloatLiteral(literal_value))
            else:
                raise Exception(f"unknown literal type {type_from_pytype(literal_value)}")
            return
        
        if isinstance(expression, parser.UnaryOperator):
            self.generate(expression.operand, script)
            if expression.operator.value == '-':
                if self.type_info.expression_to_type[expression] == 'int':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_NEGATE))
                else:
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_NEGATEF))

            if expression.operator.value == 'not':
                script.steps.append(ExpressionCommand(EXPRESSION_TYPE_NOT))

        if isinstance(expression, parser.BinaryOperator):
            operator = expression.operator.value

            cast_to_type = self.type_info.expression_to_type[expression]
            a_type = self.type_info.expression_to_type[expression.a]
            b_type = self.type_info.expression_to_type[expression.a]

            if operator in compare_operators and (a_type == 'float' or b_type == 'float'):
                cast_to_type = 'float'


            if operator in mirrored_operators:
                operator = mirrored_operators[operator]
                self.generate_to_type(expression.a, cast_to_type, script)
                self.generate_to_type(expression.b, cast_to_type, script)
            else:
                self.generate_to_type(expression.b, cast_to_type, script)
                self.generate_to_type(expression.a, cast_to_type, script)

            if operator == 'and':
                script.steps.append(ExpressionCommand(EXPRESSION_TYPE_AND))
            if operator == 'or':
                script.steps.append(ExpressionCommand(EXPRESSION_TYPE_OR))

            if operator == '==':
                script.steps.append(ExpressionCommand(EXPRESSION_TYPE_EQ))
            if operator == '!=':
                script.steps.append(ExpressionCommand(EXPRESSION_TYPE_NEQ))

            if cast_to_type == 'int':
                if operator == '>':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_GT))
                if operator == '>=':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_GTE))
                    
                if operator == '+':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_ADD))
                if operator == '-':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_SUB))
                if operator == '*':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_MUL))
                if operator == '/':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_DIV))
            else:
                if operator == '>':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_GTF))
                if operator == '>=':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_GTEF))

                if operator == '+':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_ADDF))
                if operator == '-':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_SUBF))
                if operator == '*':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_MULF))
                if operator == '/':
                    script.steps.append(ExpressionCommand(EXPRESSION_TYPE_DIVF))

        if isinstance(expression, parser.Identifier):
            name = expression.name.value
            is_global = False
            data_type = self.context.get_variable_type(name)

            if not data_type:
                raise Exception(expression.name.format_message('Variable not found'))
            
            offset = self.context.get_variable_offset(name)

            script.steps.append(ExpresionScriptLoad(is_global, name, data_type, offset))


def generate_script(expression, context: variable_layout.VariableContext) -> ExpressionScript | None:
    type_info = TypeChecker(context)
    type_info.determine_type(expression)

    if len(type_info.errors):
        print('\n\n'.join(type_info.errors))
        return None
    
    static_evaluator = StaticEvaluator()
    static_evaluator.check_for_literals(expression)

    generator = ExpressionGenerator(context, type_info, static_evaluator)

    result = ExpressionScript()
    generator.generate(expression, result)
    return result