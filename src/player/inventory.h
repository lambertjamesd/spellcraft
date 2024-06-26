#ifndef __PLAYER_INVENTORY_H__
#define __PLAYER_INVENTORY_H__

#include "../spell/spell.h"
#include "../scene/world_definition.h"

#define MAX_SPELL_SLOTS 4

#define MAX_CUSTOM_SPELLS   6

#define INVENTORY_SPELL_COLUMNS   6
#define INVENTORY_SPELL_ROWS      3

#define SPELL_SYMBOL_TO_MASK(symbol_index)  (1 << (symbol_index))

struct inventory {
    struct spell* spell_slots[MAX_SPELL_SLOTS];

    struct spell* built_in_spells[INVENTORY_SPELL_COLUMNS * INVENTORY_SPELL_ROWS];

    struct spell custom_spells[MAX_CUSTOM_SPELLS];

    uint32_t unlocked_spell_symbols;
};

void inventory_init();
void inventory_destroy();

bool inventory_has_rune(enum spell_symbol_type type);
void inventory_unlock_rune(enum spell_symbol_type type);

struct spell* inventory_get_equipped_spell(unsigned index);
void inventory_set_equipped_spell(unsigned index, struct spell* spell);

struct spell* inventory_get_built_in_spell(unsigned x, unsigned y);
struct spell* inventory_get_custom_spell(unsigned index);

#endif