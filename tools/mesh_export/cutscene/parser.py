import sys
from . import tokenizer


class _ParseState():
    def __init__(self, tokens: list[tokenizer.Token], content: str, filename: str):
        self._tokens: list[tokenizer.Token] = tokens
        self.filename: str = filename
        self.current: int = 0
        self.source = tokenizer.Source(content, filename)

    def error(self, message: str, at: int):
        print(self.source.format_message(message, at))
        sys.exit(1)

    def peek(self, offset = 0, include_whitespace = False) -> tokenizer.Token:
        skip_count = offset
        index = self.current

        while skip_count >= 0:
            if index >= len(self._tokens):
                break
            
            result = self._tokens[index]

            if not include_whitespace and result.token_type == 'whitespace':
                index += 1
            elif skip_count == 0:
                return result
            else:
                index += 1
                skip_count -= 1
            
        return self._tokens[-1]
        
    
    def advance(self, include_whitespace = False):
        if not include_whitespace and self.peek(include_whitespace=True).token_type == 'whitespace':
            self.current += 1

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
    def __init__(self, start_token: tokenizer.Token, contents: list[str], replacements: list):
        self.start_token: tokenizer.Token = start_token
        self.contents: list[str] = contents
        self.replacements: list = replacements

    def __str__(self):
        parts: list[str] = []

        for idx, replacement in enumerate(self.replacements):
            parts.append(self.contents[idx])
            parts.append('{' + str(replacement) + '}')

        if len(self.contents):
            parts.append(self.contents[-1])

        return ''.join(parts)

class UnaryOperator():
    def __init__(self, operator: tokenizer.Token, operand):
        self.operator: tokenizer.Token = operator
        self.operand = operand

    def __str__(self):
        return f"{self.operator.value}{self.operand}"

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

def _parse_string(parse_state: _ParseState) -> String:
    start_token = parse_state.require('"')
    current_content: list[str] = []
    contents: list[str] = []
    replacements: list = []

    next = parse_state.peek(include_whitespace=True)

    while next.token_type != '"':
        if next.token_type == 'eof':
            parse_state.error('string not terminated', start_token.at)

        if next.token_type == '{':
            parse_state.advance()
            contents.append(''.join(current_content))
            current_content = []
            replacements.append(_parse_expression(parse_state))
            parse_state.require('}')
        elif next.token_type == '\\':
            parse_state.advance()

            next = parse_state.peek(include_whitespace=True)

            if next.token_type == '"':
                current_content.append('"')
                parse_state.advance()
        else:
            current_content.append(next.value)
            parse_state.advance(include_whitespace=True)

        next = parse_state.peek(include_whitespace=True)

    parse_state.advance()
    contents.append(''.join(current_content))

    result = String(start_token, contents, replacements)
    print(result)
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
    
    if next.token_type == '"':
        return _parse_string(parse_state)
    
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

        if next_priority < priority:
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
    parse_state = _ParseState(tokenizer.tokenize(content, filename), content, filename)
    return _parse_cutscene(parse_state)

def parse_type(content: str) -> DataType:
    parse_state = _ParseState(tokenizer.tokenize(content, ''), content, '')
    return _parse_type(parse_state)
