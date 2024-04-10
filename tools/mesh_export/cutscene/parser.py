import sys
from . import tokenizer


class _ParseState():
    def __init__(self, tokens: list[tokenizer.Token], content: str, filename: str):
        self.tokens: list[tokenizer.Token] = tokens
        self.content: str = content
        self.filename: str = filename
        self.current: int = 0

    def error(self, message: str, at: int):
        print(tokenizer.format_message(message, self.content, at))
        sys.exit(1)

    def peek(self, offset = 0) -> tokenizer.Token:
        index = self.current + offset

        if index < len(self.tokens):
            return self.tokens[index]
        
        return self.tokens[-1]
    
    def advance(self):
        self.current += 1

    def require(self, token_type: str, value: str | None = None) -> tokenizer.Token:
        next = self.peek()
        if next.token_type != token_type:
            self.error(f"expected '{token_type}' got '{next.token_type}'", next.at)

        if value != None and value != next.value:
            self.error(f"expected '{value}' got '{next.value}'", next.at)

        self.advance()

        return next
    
    def optional(self, token_type: str, value: str | None = None) -> tokenizer.Token | None:
        next = self.peek()
        if next.token_type != token_type:
            return None

        if value != None and value != next.value:
            return None

        self.advance()

        return next

class DataType():
    def __init__(self, name: tokenizer.Token, count: int | None = None):
        self.name: tokenizer.Token = name
        self.count: int | None = count

    def __str__(self):
        if self.count:
            return f"{self.name.value}[{self.count}]"

        return f"{self.name.value}"
    
    def __eq__(self, other):
        if not isinstance(other, DataType):
            return False
        
        return self.name.value == other.name.value and self.count == other.count

class VariableDefinition():
    def __init__(self, name: tokenizer.Token, type: DataType):
        self.name: tokenizer.Token = name
        self.type: DataType = type

    def __str__(self):
        return f"{self.name.value}: {self.type};"


class IfStatement():
    def __init__(self, condition, statements: list):
        self.condition = condition
        self.statements = statements

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}if {self.condition} then")
        for statement in self.statements:
            statement.append_string(result, depth + 1)
        result.append(f"{space}end")
        

class Assignment():
    def __init__(self, name: tokenizer.Token, value):
        self.name: tokenizer.Token = name
        self.value = value

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}{self.name.value} = {self.value};")

class CutsceneStep():
    def __init__(self, name: tokenizer.Token, parameters):
        self.name: tokenizer.Token = name
        self.parameters = parameters

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        if len(self.parameters) == 0:
            result.append(f"{space}{self.name.value};")
        else:
            parameters = [str(parameter) for parameter in self.parameters]
            result.append(f"{space}{self.name.value} {', '.join(parameters)};")

class Identifier():
    def __init__(self, name: tokenizer.Token):
        self.name: tokenizer.Token = name

    def __str__(self):
        return self.name.value

class Integer():
    def __init__(self, value: tokenizer.Token):
        self.value: tokenizer.Token = value

    def __str__(self):
        return self.value.value

class Float():
    def __init__(self, value: tokenizer.Token):
        self.value: tokenizer.Token = value

    def __str__(self):
        return self.value.value

class String():
    def __init__(self, value: tokenizer.Token):
        self.value = value

    def __str__(self):
        return self.value.value

class UnaryOperator():
    def __init__(self, operator: tokenizer.Token, expression):
        self.operator: tokenizer.Token = operator
        self.expression = expression

    def __str__(self):
        return f"{self.operator.value}{self.expression}"

class BinaryOperator():
    def __init__(self, a, operator: tokenizer.Token, b):
        self.a = a
        self.operator: tokenizer.Token = operator
        self.b = b

    def __str__(self):
        return f"({self.a} {self.operator.value} {self.b})"

class Cutscene():
    def __init__(self):
        self.globals: list[VariableDefinition] = []
        self.locals: list[VariableDefinition] = []
        self.statements = []

    def __str__(self):
        parts: list[str] = []

        for variable in self.globals:
            parts.append('global ' +str(variable))

        for variable in self.locals:
            parts.append('local ' + str(variable))

        for statement in self.statements:
            statement.append_string(parts, 0)

        return '\n'.join(parts)

def _parse_type(parse_state: _ParseState):
    name = parse_state.require('identifier')
    count = None
    if parse_state.optional('['):
        count = int(parse_state.require('int').value)
        parse_state.require(']')
    return DataType(name, count)

def _parse_global(parse_state: _ParseState):
    parse_state.require('identifier', 'global')
    name = parse_state.require('identifier')
    parse_state.require(':')
    type = _parse_type(parse_state)
    parse_state.require(';')
    return VariableDefinition(name, type)
    
def _parse_local(parse_state: _ParseState):
    parse_state.require('identifier', 'local')
    name = parse_state.require('identifier')
    parse_state.require(':')
    type = _parse_type(parse_state)
    parse_state.require(';')
    return VariableDefinition(name, type)
    

def _maybe_parse_cutscene_def(parse_state: _ParseState, into: Cutscene) -> bool:
    next = parse_state.peek()

    if next.value == 'global':
        into.globals.append(_parse_global(parse_state))
        return True
    if next.value == 'local':
        into.locals.append(_parse_local(parse_state))
        return True
    
    return False

def _parse_block(parse_state: _ParseState):
    next = parse_state.peek()

    result = []

    while next.token_type != 'eof' and next.value != 'end':
        result.append(_parse_statement(parse_state))
        next = parse_state.peek()

    parse_state.require('identifier', 'end')

    return result

def _parse_single(parse_state: _ParseState):
    next = parse_state.peek()

    if next.token_type == 'identifier':
        parse_state.advance()
        return Identifier(next)
    
    if next.token_type == 'int':
        parse_state.advance()
        return Integer(next)
    
    if next.token_type == 'float':
        parse_state.advance()
        return Float(next)
    
    if next.token_type == 'str':
        parse_state.advance()
        return String(next)
    
    if next.token_type == '(':
        parse_state.advance()
        result = _parse_expression(parse_state)
        parse_state.require(')')
        return result

    parse_state.error('expected expression', next.at)

unary_operators = {'-', 'not'}

def _parse_unary(parse_state: _ParseState):
    next = parse_state.peek()

    if next.value in unary_operators:
        parse_state.advance()
        return UnaryOperator(next, _parse_single(parse_state))
    
    return _parse_single(parse_state)

operator_priority = {
    'or': 1,
    'and': 2,
    '==': 3,
    '!=': 3,
    '>': 4,
    '<': 4,
    '>=': 4,
    '<=': 4,
    '+': 5,
    '-': 5,
    '*': 6,
    '/': 6,
}

def _parse_binary(parse_state: _ParseState, priority: int):
    result = _parse_unary(parse_state)

    next = parse_state.peek()

    while next.value in operator_priority:
        next_priority = operator_priority[next.value]

        if next_priority >= priority:
            return result
        
        parse_state.advance()

        right = _parse_binary(parse_state, next_priority)

        result = BinaryOperator(result, next, right)

        next = parse_state.peek()
        
    return result


def _parse_expression(parse_state: _ParseState):
    return _parse_binary(parse_state, 0)

def _parse_if(parse_state: _ParseState):
    parse_state.require('identifier', 'if')
    condition = _parse_expression(parse_state)
    parse_state.require('identifier', 'then')
    body = _parse_block(parse_state)
    return IfStatement(condition, body)
    

def _is_assignment(parse_state: _ParseState):
    return parse_state.peek(0).token_type == 'identifier' and parse_state.peek(1).token_type == '='

def _parse_assignment(parse_state: _ParseState):
    name = parse_state.require('identifier')
    parse_state.require('=')
    expression = _parse_expression(parse_state)
    parse_state.require(';')
    return Assignment(name, expression)

def _parse_step(parse_state: _ParseState):
    name = parse_state.require('identifier')
    parameters = []

    while not parse_state.optional(';'):
        parameters.append(_parse_expression(parse_state))
        if not parse_state.optional(','):
            parse_state.require(';')
            break

    return CutsceneStep(name, parameters)

def _parse_statement(parse_state: _ParseState):
    next = parse_state.peek()

    if next.value == 'if':
        return _parse_if(parse_state)
    
    if _is_assignment(parse_state):
        return _parse_assignment(parse_state)
    
    return _parse_step(parse_state)

def _parse_cutscene(parse_state: _ParseState) -> Cutscene:
    result = Cutscene()

    while parse_state.peek().token_type != 'eof':
        if _maybe_parse_cutscene_def(parse_state, result):
            continue

        result.statements.append(_parse_statement(parse_state))

    return result

def parse(content: str, filename: str) -> Cutscene:
    parse_state = _ParseState(tokenizer.tokenize(content), content, filename)
    return _parse_cutscene(parse_state)