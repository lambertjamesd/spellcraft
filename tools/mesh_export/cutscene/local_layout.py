
from . import parser

class LocalLayout:
    def __init__(self, block: list):
        self._var_to_pos: dict[str, int] = {}

    def start_block(self, block: list):
        pass

    def end_block(self):
        pass

    def define_local(self, definition: parser.VariableDefinition):
        self._var_to_pos[definition.name.value] = 0

    def get_local_stack_position(self, name: str) -> int | None:
        if name in self._var_to_pos:
            return self._var_to_pos[name]

        return None
    
    