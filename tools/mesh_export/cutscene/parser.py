import sys
import typing
from . import tokenizer

skippable = {'whitespace', 'comment'}

class ParseError(Exception):
    def __init__(self, message: str, source: tokenizer.Source, at: int):
        super().__init__(source.format_message(message, at))
        self.message = message
        self.source = source
        self.at = at

class _ParseState():
    def __init__(self, tokens: list[tokenizer.Token], content: str, filename: str, start_line = 1, start_col = 1):
        self._tokens: list[tokenizer.Token] = tokens
        self.filename: str = filename
        self.current: int = 0
        self.source = tokenizer.Source(content, filename, start_line=start_line, start_col=start_col)

    def error(self, message: str, at: int):
        raise ParseError(message, self.source, at)

    def peek(self, offset = 0, include_whitespace = False) -> tokenizer.Token:
        skip_count = offset
        index = self.current

        while skip_count >= 0:
            if index >= len(self._tokens):
                break
            
            result = self._tokens[index]

            if not include_whitespace and result.token_type in skippable:
                index += 1
            elif skip_count == 0:
                return result
            else:
                index += 1
                skip_count -= 1
            
        return self._tokens[-1]
        
    
    def advance(self, include_whitespace = False):
        while not include_whitespace and self.peek(include_whitespace=True).token_type in skippable:
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
    
    def get_eval_type(self) -> str:
        if self.name.value == 'float':
            return 'float'
        
        return 'int'

class Expression():
    def __init__(self, at: tokenizer.Token):
        self.at: tokenizer.Token = at

class VariableDefinition():
    def __init__(self, name: tokenizer.Token, type: DataType, initializer: Expression | None):
        self.name: tokenizer.Token = name
        self.type: DataType = type
        self.initializer: Expression | None = initializer

    def __str__(self):
        return f"{self.name.value}: {self.type};"
    
    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}{self.name.value}: {self.type};")

class IfStatement():
    def __init__(self, condition: Expression, statements: list["Statement"], else_block: list["Statement"] | None):
        self.condition: Expression = condition
        self.statements: list[Statement] = statements
        self.else_block: list[Statement] | None = else_block

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}if {self.condition} then")
        for statement in self.statements:
            statement.append_string(result, depth + 1)
        if self.else_block:
            result.append(f"{space}else")
            for statement in self.else_block:
                statement.append_string(result, depth + 1)

        result.append(f"{space}end")
        

class Assignment():
    def __init__(self, left: list[tokenizer.Token], right: list[Expression]):
        self.left: list[tokenizer.Token] = left
        self.right: list[Expression] = right

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}{', '.join([name.value for name in self.left])} = {', '.join([str(value) for value in self.right])};")

class ReturnStatement():
    def __init__(self, return_token: tokenizer.Token, results: list[Expression]):
        self.return_token: tokenizer.Token = return_token
        self.results: list[Expression] = results
        
    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}return {', '.join([str(result) for result in self.results])};")


class FunctionDefinitionArg():
    def __init__(self, name: tokenizer.Token, type_name: DataType):
        self.name: tokenizer.Token = name
        self.type_name: DataType = type_name

    def __str__(self):
        return f"${self.name.value}: ${self.type_name.name.value}"

class FunctionDefinition():
    def __init__(self, func: tokenizer.Token, name: tokenizer.Token, args: list[FunctionDefinitionArg], return_types: list[DataType], body: list["Statement"], built_in = False):
        self.func: tokenizer.Token = func
        self.name: tokenizer.Token = name
        self.args: list[FunctionDefinitionArg] = args
        self.return_types: list[DataType] = return_types
        self.body: list[Statement] = body
        self.built_in: bool = built_in

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        result.append(f"{space}func ${self.name.value}(${', '.join([str(arg) for arg in self.args])})")

        for step in self.body:
            step.append_string(result, depth + 1)

        result.append(f"{space}end")
        
class FunctionCall(Expression):
    def __init__(self, name: tokenizer.Token, args: list[Expression]):
        super().__init__(name)
        self.name: tokenizer.Token = name
        self.args: list[Expression] = args

    def __str__(self) -> str:
        return f"{self.name.value}({', '.join([str(arg) for arg in self.args])})"

class CutsceneStep():
    def __init__(self, name: tokenizer.Token, parameters: list[Expression]):
        self.name: tokenizer.Token = name
        self.parameters: list[Expression] = parameters

    def append_string(self, result: list[str], depth: int):
        space = '  ' * depth
        if len(self.parameters) == 0:
            result.append(f"{space}{self.name.value};")
        else:
            parameters = [str(parameter) for parameter in self.parameters]
            result.append(f"{space}{self.name.value} {', '.join(parameters)};")

class Identifier(Expression):
    def __init__(self, name: tokenizer.Token):
        super().__init__(name)
        self.name: tokenizer.Token = name

    def __str__(self):
        return self.name.value

class Integer(Expression):
    def __init__(self, value: tokenizer.Token):
        super().__init__(value)
        self.value: tokenizer.Token = value

    def __str__(self):
        return self.value.value

class Float(Expression):
    def __init__(self, value: tokenizer.Token):
        super().__init__(value)
        self.value: tokenizer.Token = value

    def __str__(self):
        return self.value.value

class StringReplacement():
    def __init__(self, expr: Expression, format: tokenizer.Token | None):
        self.expr: Expression = expr
        self.format: tokenizer.Token | None = format
        
    def __str__(self):
        if self.format:
            return f"{str(self.expr)}:{self.format.value}"
        return str(self.expr)


class String(Expression):
    def __init__(self, start_token: tokenizer.Token, contents: list[str], replacements: list[StringReplacement]):
        super().__init__(start_token)
        self.start_token: tokenizer.Token = start_token
        self.contents: list[str] = contents
        self.replacements: list[StringReplacement] = replacements

    def __str__(self):
        parts: list[str] = []

        for idx, replacement in enumerate(self.replacements):
            parts.append(self.contents[idx])
            parts.append('{' + str(replacement) + '}')

        if len(self.contents):
            parts.append(self.contents[-1])

        return ''.join(parts)

class UnaryOperator(Expression):
    def __init__(self, operator: tokenizer.Token, operand: Expression):
        super().__init__(operator)
        self.at: tokenizer.Token = operator
        self.operator: tokenizer.Token = operator
        self.operand: Expression = operand

    def __str__(self):
        return f"{self.operator.value} {self.operand}"

class BinaryOperator(Expression):
    def __init__(self, a: Expression, operator: tokenizer.Token, b: Expression):
        super().__init__(operator)
        self.a: Expression = a
        self.operator: tokenizer.Token = operator
        self.b: Expression = b

    def __str__(self):
        return f"({self.a} {self.operator.value} {self.b})"

class Cutscene():
    def __init__(self):
        self.globals: list[VariableDefinition] = []
        self.scene_vars: list[VariableDefinition] = []
        self.functions: list[FunctionDefinition] = []

    def __str__(self):
        parts: list[str] = []

        for variable in self.globals:
            parts.append('global ' + str(variable))

        for scene_var in self.scene_vars:
            parts.append('scene ' + str(scene_var))

        for fn in self.functions:
            fn.append_string(parts, 0)

        return '\n'.join(parts)
    

Statement = typing.Union[VariableDefinition, IfStatement, Assignment, FunctionDefinition, CutsceneStep, ReturnStatement]
ExpressionUnion = typing.Union[FunctionCall, Identifier, Integer, Float, String, UnaryOperator, BinaryOperator]

def _parse_type(parse_state: _ParseState) -> DataType:
    name = parse_state.require('identifier')
    count = None
    if parse_state.optional('['):
        count = int(parse_state.require('int').value)
        parse_state.require(']')
    return DataType(name, count)

def _parse_variable_definition(parse_state: _ParseState, type: str):
    parse_state.require('identifier', type)
    name = parse_state.require('identifier')
    parse_state.require(':')
    parsed_type = _parse_type(parse_state)
    initializer = None
    if parse_state.optional('='):
        initializer = _parse_expression(parse_state)
    parse_state.require(';')
    return VariableDefinition(name, parsed_type, initializer)

def _parse_function_definition(parse_state: _ParseState) -> FunctionDefinition:
    func = parse_state.require('identifier', 'func')
    name = parse_state.require('identifier')
    parse_state.require('(')

    args: list[FunctionDefinitionArg] = []
    
    next = parse_state.peek()
    while next.token_type != 'eof' and next.value != ')':
        arg_name = parse_state.require('identifier')
        parse_state.require(':')
        arg_type_name = _parse_type(parse_state)

        args.append(FunctionDefinitionArg(arg_name, arg_type_name))
        next = parse_state.peek()

        if next.value != ')':
            parse_state.require(',')

    parse_state.require(')')

    expect_return_type = bool(parse_state.optional(':'))

    return_types: list[DataType] = []

    while expect_return_type:
        return_types.append(_parse_type(parse_state))
        expect_return_type = bool(parse_state.optional(','))


    body = _parse_block(parse_state, {'end'})
    parse_state.require('identifier', 'end')

    return FunctionDefinition(func, name, args, return_types, body)

def _maybe_parse_cutscene_def(parse_state: _ParseState, into: Cutscene) -> bool:
    next = parse_state.peek()

    if next.value == 'global':
        into.globals.append(_parse_variable_definition(parse_state, 'global'))
        return True
    if next.value == 'scene':
        into.scene_vars.append(_parse_variable_definition(parse_state, 'scene'))
        return True
    if next.value == 'func':
        into.functions.append(_parse_function_definition(parse_state))
        return True
    
    return False

def _parse_block(parse_state: _ParseState, block_terminators: set[str]):
    next = parse_state.peek()

    result = []

    while next.token_type != 'eof' and not next.value in block_terminators:
        result.append(_parse_statement(parse_state))
        next = parse_state.peek()

    return result

def _adjust_string_whitespace(contents: list[str]):
    first_str = contents[0]

    if len(first_str) == 0 or first_str[0] != '\n':
        return
    
    idx = 1
    while idx < len(first_str) and first_str[idx].isspace() and first_str[idx] != '\n':
        idx += 1
        
    white_space_prefix = first_str[1:idx]

    content_index = 0
    contents[0] = first_str[idx:]
    whitepsace_index = None

    current_output: list[str] = []

    while content_index < len(contents):
        current_str = contents[content_index]

        for idx in range(len(current_str)):
            curr = current_str[idx]

            if whitepsace_index == None or whitepsace_index >= len(white_space_prefix) or white_space_prefix[whitepsace_index] != curr:
                current_output.append(curr)
                whitepsace_index = None
            else:
                whitepsace_index += 1

            if curr == '\n':
                whitepsace_index = 0

        contents[content_index] = ''.join(current_output)
        current_output = []

        content_index += 1


def _parse_string(parse_state: _ParseState) -> String:
    start_token = parse_state.require('"')
    current_content: list[str] = []
    contents: list[str] = []
    replacements: list[StringReplacement] = []

    next = parse_state.peek(include_whitespace=True)

    while next.token_type != '"':
        if next.token_type == 'eof':
            parse_state.error('string not terminated', start_token.at)

        if next.token_type == '{':
            parse_state.advance()
            contents.append(''.join(current_content))
            current_content = []
            expr = _parse_expression(parse_state)
            format = None

            if parse_state.optional(':'):
                format = parse_state.require('identifier')

            replacements.append(StringReplacement(expr, format))
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

    _adjust_string_whitespace(contents)

    return String(start_token, contents, replacements)

def _parse_function_call(parse_state: _ParseState):
    name = parse_state.require('identifier')
    parse_state.require('(')
    args = []

    while not parse_state.optional(')'):
        args.append(_parse_expression(parse_state))

        next = parse_state.peek()

        if next.token_type != ')':
            parse_state.require(',')

    return FunctionCall(name, args)

def _parse_single(parse_state: _ParseState):
    next = parse_state.peek()

    if next.token_type == 'identifier':
        if parse_state.peek(1).token_type == '(':
            return _parse_function_call(parse_state)

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

def _parse_if(parse_state: _ParseState, if_token = 'if'):
    parse_state.require('identifier', if_token)
    condition = _parse_expression(parse_state)
    parse_state.require('identifier', 'then')
    body = _parse_block(parse_state, {'elif', 'else', 'end'})
    else_block = None

    if parse_state.optional('identifier', 'else'):
        else_block = _parse_block(parse_state, {'end'})
    elif parse_state.peek().value == 'elif':
        else_block = [_parse_if(parse_state, 'elif')]

    if if_token == 'if':
        parse_state.require('identifier', 'end')

    return IfStatement(condition, body, else_block)

def _is_assignment(parse_state: _ParseState):
    offset = 0

    while parse_state.peek(offset).token_type == 'identifier':
        maybe_assign = parse_state.peek(offset+1).token_type

        if maybe_assign == '=':
            return True
        elif maybe_assign == ',':
            offset += 2
        else:
            return False

    return False

def _parse_assignment(parse_state: _ParseState):
    left = [parse_state.require('identifier')]

    while parse_state.optional(','):
        left.append(parse_state.require('identifier'))

    parse_state.require('=')
    right: list[Expression] = [_parse_expression(parse_state)]

    while parse_state.optional(','):
        right.append(_parse_expression(parse_state))

    parse_state.require(';')
    return Assignment(left, right)

def _parse_return_statement(parse_state: _ParseState):
    return_token = parse_state.require('identifier', 'return')

    results: list[Expression] = []
    
    while not parse_state.optional(';'):
        results.append(_parse_expression(parse_state))
        if not parse_state.optional(','):
            parse_state.require(';')
            break

    return ReturnStatement(return_token, results)


def _parse_step(parse_state: _ParseState):
    name = parse_state.require('identifier')
    parameters = []

    while not parse_state.optional(';'):
        parameters.append(_parse_expression(parse_state))
        if not parse_state.optional(','):
            parse_state.require(';')
            break

    return CutsceneStep(name, parameters)

def _parse_statement(parse_state: _ParseState) -> Statement:
    next = parse_state.peek()

    if next.value == 'if':
        return _parse_if(parse_state)
    
    if _is_assignment(parse_state):
        return _parse_assignment(parse_state)
    
    if next.value == 'local':
        return _parse_variable_definition(parse_state, 'local')
    
    if next.value == 'return':
        return _parse_return_statement(parse_state)
    
    return _parse_step(parse_state)

def _parse_cutscene(parse_state: _ParseState) -> Cutscene:
    result = Cutscene()

    while parse_state.peek().token_type != 'eof':
        if _maybe_parse_cutscene_def(parse_state, result):
            continue

        next = parse_state.peek()

        parse_state.error(f'expected variable or function def got {next.value}', next.at)

        result.statements.append(_parse_statement(parse_state))

    result.functions.sort(key=lambda x: (0 if x.name.value == 'main' else 1, x.name.value))

    return result

def parse(content: str, filename: str) -> Cutscene:
    parse_state = _ParseState(tokenizer.tokenize(content, filename), content, filename)
    return _parse_cutscene(parse_state)

def parse_type(content: str) -> DataType:
    parse_state = _ParseState(tokenizer.tokenize(content, ''), content, '')
    return _parse_type(parse_state)

def parse_expression(content: str, source: str):
    parse_state = _ParseState(tokenizer.tokenize(content, source), content, source)
    return _parse_expression(parse_state)

def parse_block(content: str, source: str) -> list[Statement]:
    parse_state = _ParseState(tokenizer.tokenize(content, source), content, source)
    return _parse_block(parse_state, set())

def statement_list_str(block: list[Statement]):
    result: list[str] = []

    for statement in block:
        statement.append_string(result, 0)

    return '\n'.join(result)

def parse_function_definition(content: str, source: str, start_line = 1, start_col = 1):
    parse_state = _ParseState(
        tokenizer.tokenize(content, source, start_line = start_line, start_col = start_col), 
        content, source, 
        start_line = start_line, start_col = start_col
    )
    return _parse_function_definition(parse_state)