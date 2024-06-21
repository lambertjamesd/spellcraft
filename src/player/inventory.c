#include "inventory.h"

#include <memory.h>
#include "../util/flags.h"
#include <assert.h>

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

struct inventory inventory;

void inventory_init() {
    memset(inventory.spell_slots, 0, sizeof(inventory.spell_slots));

    memset(inventory.built_in_spells, 0, sizeof(inventory.built_in_spells));

    inventory.built_in_spells[0] = &flame_spell;
    inventory.built_in_spells[1] = &dash_spell;

    inventory.spell_slots[0] = &flame_spell;

    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_init(&inventory.custom_spells[i], SPELL_MAX_COLS, SPELL_MAX_ROWS, SPELL_ICON_CUSTOM_0 + i);
    }
}

void inventory_destroy() {
    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_destroy(&inventory.custom_spells[i]);
    }
}

bool inventory_has_rune(enum spell_symbol_type type) {
    return true; // HAS_FLAG(inventory.unlocked_spell_symbols, SPELL_SYMBOL_TO_MASK(type)); 
}

void inventory_unlock_rune(enum spell_symbol_type type) {
    SET_FLAG(inventory.unlocked_spell_symbols, SPELL_SYMBOL_TO_MASK(type));
}

struct spell* inventory_get_equipped_spell(unsigned index) {
    assert(index < MAX_SPELL_SLOTS);
    return inventory.spell_slots[index];
}

void inventory_set_equipped_spell(unsigned index, struct spell* spell) {
    assert(index < MAX_SPELL_SLOTS);
    inventory.spell_slots[index] = spell;
}

struct spell* inventory_get_built_in_spell(unsigned x, unsigned y) {
    assert(x < INVENTORY_SPELL_COLUMNS && y < INVENTORY_SPELL_ROWS);
    return inventory.built_in_spells[
        y * INVENTORY_SPELL_COLUMNS + x
    ];
}

struct spell* inventory_get_custom_spell(unsigned index) {
    assert(index < MAX_CUSTOM_SPELLS);
    return &inventory.custom_spells[index];
}