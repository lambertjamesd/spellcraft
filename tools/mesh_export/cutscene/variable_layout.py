import sys
from . import parser

_type_bit_sizes = {
    'char': 8,
    'bool': 1,
    'i8': 8,
    'i16': 16,
    'i32': 32,
    'float': 32,
}

def _determine_type_bit_size(type: parser.DataType, filename: str):
    if not type.name.value in _type_bit_sizes:
        print(type.name.format_message('invalid type', filename))
        sys.exit(1)

    result = _type_bit_sizes[type.name.value]
    alignment = result

    if type.count:
        result *= type.count

    return result, alignment

class VariableLayout():
    def __init__(self):
        self.bit_used: list[bool] = []
        self.variable_bit_offset: dict[str, int] = {}
        self.variable_definition: dict[str, parser.VariableDefinition] = {}
        self.variable_filename: dict[str, str] = {}

    def add_variable(self, variable: parser.VariableDefinition, filename: str) -> bool:
        name_str = variable.name.value
        
        if name_str in self.variable_definition:
            existing = self.variable_definition[name_str]

            if not variable.type == str(existing.type):
                print(variable.name.format_message(f'redefinition of variable with mismatched type', filename))
                print(existing.name.format_message(f'previous definiton was here', filename))
                return False

            return True
        
        self.variable_definition[name_str] = variable
        self.variable_filename[name_str] = filename

        bit_size, alignment = _determine_type_bit_size(variable.type, filename)

        offset = 0

        while True:
            is_valid = True

            for idx in range(bit_size):
                check = offset + idx

                if check >= len(self.bit_used):
                    break
                elif self.bit_used[check]:
                    is_valid = False
                    break

            if is_valid:
                break

            offset += alignment

        self.variable_bit_offset[name_str] = offset

        for idx in range(bit_size):
            check = offset + idx

            if check < len(self.bit_used):
                self.bit_used.append(True)
            else
                self.bit_used[check] = True


    