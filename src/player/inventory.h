#ifndef __PLAYER_INVENTORY_H__
#define __PLAYER_INVENTORY_H__

#include "../spell/spell.h"
#include "../scene/scene_definition.h"
#include "../cutscene/evaluation_context.h"
#include "staff.h"

#define MAX_SPELL_SLOTS 4

#define MAX_CUSTOM_SPELLS   6

#define INVENTORY_SPELL_COLUMNS   6
#define INVENTORY_SPELL_ROWS      3

#define SPELL_SYMBOL_TO_MASK(symbol_index)  (1 << (symbol_index))

#define EQUIPPED_NONE   0xFF

extern enum inventory_item_type staff_item_types[INV_STAFF_COUNT];

struct inventory {
    struct spell* spell_slots[MAX_SPELL_SLOTS];

    struct spell* built_in_spells[INVENTORY_SPELL_COLUMNS * INVENTORY_SPELL_ROWS];

    struct spell custom_spells[MAX_CUSTOM_SPELLS];

    struct staff_stats* equipped_staff;
};

struct global_location { 
    uint16_t data_type;
    uint16_t word_offset;
};

void inventory_init();
void inventory_destroy();

bool inventory_has_item(enum inventory_item_type type);
void inventory_unlock_item(enum inventory_item_type type);

int inventory_get_item_level(enum inventory_item_type type);

struct staff_stats* inventory_equipped_staff();
void inventory_equip_staff(enum inventory_item_type type);

struct spell* inventory_get_equipped_spell(unsigned index);
void inventory_set_equipped_spell(unsigned index, struct spell* spell);

struct spell* inventory_get_built_in_spell(unsigned x, unsigned y);
struct spell* inventory_get_custom_spell(unsigned index);

#endif