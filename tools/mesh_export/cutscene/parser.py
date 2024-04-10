import sys
from . import tokenizer

def determine_source_location(content: str, at: int) -> tuple[int, int]:
    line = 1
    col = 1

    for idx in range(at):
        if content[idx] == '\n':
            line += 1
            col = 1
        else:
            col += 1

    return line, col

def get_source_line(content: str, at: int) -> str:
    line_start = content.rfind('\n', 0, at)
    line_end = content.find('\n', at)
    return content[line_start:line_end]


class _ParseState():
    def __init__(self, tokens: list[tokenizer.Token], content: str, filename: str):
        self.tokens: list[tokenizer.Token] = tokens
        self.content: str = content
        self.filename: str = filename
        self.current: int = 0

    def error(self, message: str, at: int):
        line, col = determine_source_location(self.content, at)
        padding = ' ' * (col - 1)

        print(f'{self.filename}:{line}:{col} {message}\n{get_source_line(self.content, at)}\n{padding}^')
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
            self.error(f"expected '{token_type}' got '{next.token_type}'")

        if value != None and value != next.value:
            self.error(f"expected '{value}' got '{next.value}'")

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

class VariableDefinition():
    def __init__(self, name: tokenizer.Token, type: DataType):
        self.name: tokenizer.Token = name
        self.type: DataType = type

class IfStatement():
    def __init__(self, condition, statements: list):
        self.condition = condition
        self.statements = statements

class Cutscene():
    def __init__(self):
        self.globals: list[VariableDefinition] = []
        self.locals: list[VariableDefinition] = []
        self.statements = []

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
    return VariableDefinition(name, _parse_type(parse_state))
    
def _parse_local(parse_state: _ParseState):
    parse_state.require('identifier', 'local')
    name = parse_state.require('identifier')
    parse_state.require(':')
    return VariableDefinition(name, _parse_type(parse_state))
    

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

    parse_state.require('identifier', 'end')

    return result

def _parse_expression(parse_state: _ParseState):
    parse_state.optional('identifier')
    pass

def _parse_if(parse_state: _ParseState):
    parse_state.require('identifier', 'if')
    condition = _parse_expression(parse_state)
    parse_state.require('identifier', 'then')
    body = _parse_block(parse_state)
    return IfStatement(condition, body)
    

def _parse_statement(parse_state: _ParseState):
    next = parse_state.peek()

    if next.value == 'if':
        return _parse_if(parse_state)
    
    parse_state.error('expected statement', parse_state.peek().at)

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