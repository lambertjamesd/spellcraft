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
    {.type = SPELL_SYMBOL_FIRE},
    {.type = SPELL_SYMBOL_AIR},
};

struct spell dash_spell = {
    .symbols = dash_spell_symbols,
    .cols = 2,
    .rows = 1,

    .symbol_index = SPELL_ICON_DASH,
};

struct spell_symbol projectile_spell_symbols[] = {
    {.type = SPELL_SYMBOL_EARTH},
};

struct spell projectile_spell = {
    .symbols = projectile_spell_symbols,
    .cols = 1,
    .rows = 1,

    .symbol_index = SPELL_ICON_DASH,
};

enum inventory_item_type staff_item_types[INV_STAFF_COUNT] = {
    ITEM_TYPE_STAFF_DEFAULT,
    ITEM_TYPE_NONE,
    ITEM_TYPE_NONE,
    ITEM_TYPE_NONE,
};

struct inventory inventory;

void inventory_init() {
    memset(inventory.spell_slots, 0, sizeof(inventory.spell_slots));

    memset(inventory.built_in_spells, 0, sizeof(inventory.built_in_spells));

    inventory.built_in_spells[0] = &flame_spell;
    inventory.built_in_spells[1] = &dash_spell;

    inventory.spell_slots[0] = &flame_spell;
    inventory.spell_slots[1] = &dash_spell;
    inventory.spell_slots[2] = &projectile_spell;

    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_init(&inventory.custom_spells[i], SPELL_MAX_COLS, SPELL_MAX_ROWS, SPELL_ICON_CUSTOM_0 + i);
    }

    inventory.equipped_staff = &staff_stats_none;
}

void inventory_destroy() {
    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_destroy(&inventory.custom_spells[i]);
    }
}

bool inventory_has_item(enum inventory_item_type type) {
    if (type < SPELL_SYBMOL_COUNT) {
        return true;
    }
    
    return HAS_FLAG(inventory.unlocked_items, SPELL_SYMBOL_TO_MASK(type)); 
}

void inventory_unlock_item(enum inventory_item_type type) {
    SET_FLAG(inventory.unlocked_items, SPELL_SYMBOL_TO_MASK(type));
}

struct staff_stats* inventory_equipped_staff() {
    return inventory.equipped_staff;
}

void inventory_equip_staff(enum inventory_item_type type) {
    for (int i = 0; i < INV_STAFF_COUNT; i += 1) {
        if (staff_stats[i].item_type == type) {
            inventory.equipped_staff = &staff_stats[i];
            return;
        }
    }

    inventory.equipped_staff = &staff_stats_none;
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