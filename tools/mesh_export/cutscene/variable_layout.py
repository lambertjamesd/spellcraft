import sys
import json
from . import parser

_type_bit_sizes = {
    'char': 8,
    'bool': 1,
    'i8': 8,
    'i16': 16,
    'i32': 32,
    'float': 32,
}

def _determine_type_bit_size(type: parser.DataType):
    if not type.name.value in _type_bit_sizes:
        print(type.name.format_message('invalid type'))
        sys.exit(1)

    result = _type_bit_sizes[type.name.value]
    alignment = result

    if type.count:
        result *= type.count

    return result, alignment

class VariableInfo():
    def __init__(self, name: str, bit_size: int, alignment: int, type_name: str):
        self.name: str = name
        self.bit_size: int = bit_size
        self.alignment: int = alignment
        self.type_name: str = type_name


class VariableLayout():
    def __init__(self):
        self.entries: dict[str, int] = {}
        self.types: dict[str, str] = {}

    def deserialize(self, file):
        file_contents = json.load(file)

        for entry in file_contents['entries']:
            self.entries[entry["name"]] = entry["offset"]
            self.types[entry["name"]] = entry["type"]

    def get_variable_offset(self, name: str) -> int:
        return self.entries[name]
    
    def get_variable_type(self, name: str) -> str | None:
        if name in self.types:
            return self.types[name]
        return None

class VariableLayoutBuilder():
    def __init__(self):
        self.entries: list[VariableInfo] = []

        self.variable_definition: dict[str, parser.VariableDefinition] = {}

    def _layout(self) -> list[tuple[VariableInfo, int]]:
        result = []

        sorted_entries = sorted(self.entries, key=lambda x: x.alignment)
        
        offset = 0

        for entry in sorted_entries:
            offset = (offset + entry.alignment - 1) & ~(entry.alignment - 1)
            result.append((entry, offset))
            offset += entry.bit_size

        return result

    def __str__(self):
        entries = self._layout()
        return '\n'.join(["0x{0:04x} {1} : {2}".format(entry[1], entry[0].name, entry[0].type_name) for entry in entries])

    def add_variable(self, variable: parser.VariableDefinition) -> bool:
        name_str = variable.name.value
        
        if name_str in self.variable_definition:
            existing = self.variable_definition[name_str]

            if not variable.type == str(existing.type):
                print(variable.name.format_message(f'redefinition of variable with mismatched type'))
                print(existing.name.format_message(f'previous definiton was here'))
                return False

            return True
        
        type_str = str(variable.type)
        self.variable_definition[name_str] = variable

        bit_size, alignment = _determine_type_bit_size(variable.type)

        self.entries.append(VariableInfo(name_str, bit_size, alignment, type_str))

        return True

    def serialize(self, file):
        entries = self._layout()

        file.write(json.dumps({
            "entries": [{"name": entry[0].name, "offset": entry[1], "type": entry[0].type_name} for entry in entries]
        }, sort_keys=True, indent=4))

    def build(self) -> VariableLayout:
        entries = self._layout()

        result = VariableLayout()

        for entry in entries:
            result.entries[entry[0].name] = entry[1]
            result.types[entry[0].name] = entry[0].type_name

        return result
