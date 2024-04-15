import sys
import json
import struct
import base64
from . import parser
from . import static_evaluator

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

def _calculate_initial_value(value, type: parser.DataType):
    name = type.name.value

    if name == 'bool':
        return struct.pack('>B', 1 if value else 0)
    if name == 'i8':
        return struct.pack('>b', value or 0)
    if name == 'i16':
        return struct.pack('>h', value or 0)
    if name == 'i32':
        return struct.pack('>i', value or 0)
    if name == 'float':
        return struct.pack('>f', value or 0)
    if name == 'char':
        result = (value or '').encode()
        return result[0:type.count] + bytes(max(0, type.count - len(result)))
    raise Exception(f'Could not determine inital value for {type}')

class VariableInfo():
    def __init__(self, name: str, bit_size: int, alignment: int, type_name: str, initial_value: bytes):
        self.name: str = name
        self.bit_size: int = bit_size
        self.alignment: int = alignment
        self.type_name: str = type_name
        self.initial_value: bytes = initial_value

class VaraibleLayoutEntry():
    def __init__(self, name: str, type_name: str, offset: int, bit_size: int):
        self.name: str = name
        self.type_name: str = type_name
        self.offset: int = offset
        self.bit_size: int = bit_size

class VariableLayout():
    def __init__(self):
        self._entries: dict[str, VaraibleLayoutEntry] = {}

    def deserialize(self, file):
        file_contents = json.load(file)

        for entry in file_contents['entries']:
            self._entries[entry["name"]] = VaraibleLayoutEntry(
                entry["name"],
                entry["type"],
                entry["offset"],
                entry["bitSize"]
            )

    def get_variable_offset(self, name: str) -> int:
        return self._entries[name]
    
    def get_variable_type(self, name: str) -> str | None:
        if name in self.types:
            return self.types[name]
        return None
    
    def get_total_size(self) -> int:
        result: int = 0

        for entry in self._entries.values():
            result = max(result, entry.offset + entry.bit_size)

        return int((result + 7) // 8)
            

class VariableLayoutBuilder():
    def __init__(self):
        self._entries: list[VariableInfo] = []

        self.variable_definition: dict[str, parser.VariableDefinition] = {}

    def _layout(self) -> list[tuple[VariableInfo, int]]:
        result = []

        sorted_entries = sorted(self._entries, key=lambda x: x.alignment)
        
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

        value = None

        if variable.initializer:
            eval = static_evaluator.StaticEvaluator()

            if isinstance(variable.initializer, parser.String):
                if len(variable.initializer.replacements) > 0:
                    print(variable.initializer.at.format_message('varaibles cannot be initialized with a template string'))
                    sys.exit()

                value = variable.initializer.contents[0]
            else:
                value = eval.check_for_literals(variable.initializer)

                if not value:
                    print(variable.initializer.at.format_message('variables can only be initialized with a constant'))
                    sys.exit()

        initial_value = _calculate_initial_value(value, variable.type)

        self._entries.append(VariableInfo(name_str, bit_size, alignment, type_str, initial_value))

        return True

    def serialize(self, file):
        entries = self._layout()

        file.write(json.dumps({
            "entries": [{
                "name": entry[0].name, 
                "type": entry[0].type_name,
                "offset": entry[1], 
                "bitSize": entry[0].bit_size,
                "intialvalue": entry[0].initial_value.hex()
            } for entry in entries]
        }, sort_keys=True, indent=4))

    def build(self) -> VariableLayout:
        entries = self._layout()

        result = VariableLayout()

        for entry in entries:
            result._entries[entry[0].name] = VaraibleLayoutEntry(
                entry[0].name,
                entry[0].type_name,
                entry[1],
                entry[0].bit_size
            )

        return result

class VariableContext():
    def __init__(self, globals: VariableLayout, locals: VariableLayout):
        self.globals: VariableLayout = globals
        self.locals: VariableLayout = locals

    def is_local(self, name: str) -> bool:
        return name in self.locals

    def get_variable_offset(self, name: str) -> int:
        result = self.locals.get_variable_offset(name)

        if not result is None:
            return result
        
        return self.globals.get_variable_offset(name)
    
    def get_variable_type(self, name: str) -> str | None:
        return self.locals.get_variable_type(name) or self.globals.get_variable_type(name)