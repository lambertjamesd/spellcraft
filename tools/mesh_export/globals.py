import sys
import argparse
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.cutscene.parser
import mesh_export.cutscene.variable_layout

mapping = {
    "fire_rune_level": 'SPELL_SYMBOL_FIRE',
    "ice_rune_level": 'SPELL_SYMBOL_WATER',
    "earth_rune_level": 'SPELL_SYMBOL_EARTH',
    "air_rune_level": 'SPELL_SYMBOL_AIR',
    "life_rune_level": 'SPELL_SYMBOL_LIFE',
    "recast_rune_level": 'SPELL_SYMBOL_RECAST',
    "has_staff_default": 'ITEM_TYPE_STAFF_DEFAULT',
}

data_type_mapping = {
    "i8": "DATA_TYPE_S8",
    "i16": "DATA_TYPE_S16",
    "i32": "DATA_TYPE_S32",
    "entity_id": "DATA_TYPE_S16",
    "entity_spawner": "DATA_TYPE_S32",
    "bool": "DATA_TYPE_BOOL",
    "float": "DATA_TYPE_F32",
    "str": "DATA_TYPE_ADDRESS",
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('output')
    parser.add_argument('inventory_mapping')
    parser.add_argument('globals_header')
    parser.add_argument('input', nargs='*')

    args = parser.parse_args()
    result = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()

    success = True
    
    for input in args.input:
        with open(input) as file:
            current_cutscene = mesh_export.cutscene.parser.parse(file.read(), args.input)

            for global_var in current_cutscene.globals:
                success = result.add_variable(global_var) and success

    if not success:
        sys.exit(1)

    entries = result.build()

    with open(args.output + '.dat', 'wb') as file:
        entries.write_default_values(file)

    with open(args.output + '.json', 'w') as file:
        result.serialize(file)
    
    with open(args.inventory_mapping, 'w') as file:
        file.write('#include "inventory.h"\n')
        file.write('\n')
        file.write('struct global_location inventory_item_locations[ITEM_TYPE_COUNT] = {\n')

        for entry in entries.get_all_entries():
            if not entry.name in mapping:
                continue

            file.write('    [' + mapping[entry.name] + '] = { .data_type = ' + data_type_mapping[entry.type_name] + ', .word_offset = ' + str(entry.offset // entry.bit_size) + ' },\n')

        file.write('};')

    with open(args.globals_header, 'w') as file:
        file.write('#ifndef __CUTSCENE_GLOBAL_LIST_H__\n')
        file.write('#define __CUTSCENE_GLOBAL_LIST_H__\n')
        file.write('\n')
        file.write('#include "evaluation_context.h"\n')
        file.write('\n')
        
        for entry in entries.get_all_entries():
            word_offset = str(entry.offset // entry.bit_size)
            file.write(f"#define VAR_POS_{entry.name} {word_offset}\n")

            if entry.type_name.startswith('char['):
                file.write(f"#define VAR_TYP_{entry.name} DATA_TYPE_ADDRESS\n")
                file.write(f"#define VAR_LOC_{entry.name} (global_location_t){{.data_type = DATA_TYPE_ADDRESS, .word_offset = {word_offset}}}\n")
            else:
                file.write(f"#define VAR_TYP_{entry.name} {data_type_mapping[entry.type_name]}\n")
                file.write(f"#define VAR_LOC_{entry.name} (global_location_t){{.data_type = {data_type_mapping[entry.type_name]}, .word_offset = {word_offset}}}\n")

            file.write('\n')

        file.write('\n')
        file.write('#endif\n')

