#include "inventory.h"

#include <memory.h>

struct spell_symbol flame_spell_symbols[] = {
    {.type = SPELL_SYMBOL_FIRE},
};

struct spell flame_spell = {
    .symbols = flame_spell_symbols,
    .cols = 1,
    .rows = 1,

    .symbol_index = SPELL_ICON_FIRE,
};

struct spell_symbol dash_spell_symbols[] = {
    {.type = SPELL_SYMBOL_PUSH},
};

struct spell dash_spell = {
    .symbols = dash_spell_symbols,
    .cols = 1,
    .rows = 1,

    .symbol_index = SPELL_ICON_DASH,
};

void inventory_init(struct inventory* inventory) {
    memset(inventory->spell_slots, 0, sizeof(inventory->spell_slots));

    memset(inventory->built_in_spells, 0, sizeof(inventory->built_in_spells));

    inventory->built_in_spells[0] = &flame_spell;
    inventory->built_in_spells[1] = &dash_spell;

    inventory->spell_slots[0] = &flame_spell;

    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_init(&inventory->custom_spells[i], SPELL_MAX_COLS, SPELL_MAX_ROWS);
    }
}

void inventory_destroy(struct inventory* inventory) {
    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_destroy(&inventory->custom_spells[i]);
    }
}