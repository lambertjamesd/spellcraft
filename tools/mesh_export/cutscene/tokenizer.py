
class Token():
    def __init__(self, token_type: str, value: str, at: int):
        self.token_type = token_type
        self.value = value

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

def _error_state(current: str):
    if current == '':
        return _error_state, 'error'
    
    return _error_state, 'error'

def _identifier_state(current: str):
    if current.isalnum() or current == '_':
        return _identifier_state, None
    
    return _default_state(current), 'identifier'

def _single_character_state(token_type: str):
    def result(current: str):
        return _default_state(current), token_type
    return result

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
        return _single_character_state(current)
    
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

    return result