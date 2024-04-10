
class Token():
    def __init__(self, token_type: str, value: str, at: int):
        self.token_type: str = token_type
        self.value: str = value
        self.at: int = at

    def __str__(self):
        return f"'{self.value}':{self.token_type}"

def _whitespace_state(current: str):
    if current.isspace():
        return _whitespace_state, None
    return _default_state(current), 'whitespace'

def _string_escape_state(current: str):
    return _string_state, None

def _string_end_state(current: str):
    return _default_state(current), 'str'

def _string_state(current: str):
    if current == '\\':
        return _string_escape_state, None
    if current == '"':
        return _string_end_state, None
    if current == '':
        return _error_state, 'error'
    return _string_state, None

def _float_state(current: str):
    if current.isdigit():
        return _float_state, None
    
    return _default_state(current), 'float'

def _integer_state(current: str):
    if current.isdigit():
        return _integer_state, None
    if current == '.':
        return _float_state, None
    
    return _default_state(current), 'int'

def _error_state(current: str):
    if current == '':
        return _error_state, 'error'
    
    return _error_state, None

def _identifier_state(current: str):
    if current.isalnum() or current == '_':
        return _identifier_state, None
    
    return _default_state(current), 'identifier'

def _emit_token_state(token_type: str):
    def result(current: str):
        return _default_state(current), token_type
    return result

def _assign_or_equal_state(current: str):
    if current == '=':
        return _emit_token_state('==')
    return _default_state(current), '='

def _greater_than_state(current: str):
    if current == '=':
        return _emit_token_state('>=')
    return _default_state(current), '>'

def _less_than_state(current: str):
    if current == '=':
        return _emit_token_state('<=')
    return _default_state(current), '<'

_single_character_tokens = {
    '+', '-', '*', '/',
    ':', '[', ']', '(', ')', ','
}

def _default_state(current: str):
    if current == '"':
        return _string_state
    if current.isspace():
        return _whitespace_state
    if current.isalpha() or current == '_':
        return _identifier_state
    if current in _single_character_tokens:
        return _emit_token_state(current)
    if current.isdigit():
        return _integer_state
    if current == '.':
        return _float_state
    if current == '=':
        return _assign_or_equal_state
    if current == '>':
        return _greater_than_state
    if current == '<':
        return _less_than_state
    
    return _error_state

def tokenize(content: str) -> list[Token]:
    result: list[Token] = []
    state = _default_state(content[0])
    last_start = 0

    for idx in range(1, len(content) + 1):
        character = '' if idx == len(content) else content[idx]

        next_state, emit_token = state(character)

        if emit_token:
            if emit_token != 'whitespace':
                result.append(Token(emit_token, content[last_start:idx], last_start))

            last_start = idx

        state = next_state

    result.append(Token('eof', '', len(content)))

    return result