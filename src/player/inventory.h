#ifndef __PLAYER_INVENTORY_H__
#define __PLAYER_INVENTORY_H__

#include "../spell/spell.h"

#define MAX_SPELL_SLOTS 4

#define MAX_CUSTOM_SPELLS   8

#define SPELL_COLUMNS   6
#define SPELL_ROWS      3

struct inventory {
    struct spell* spell_slots[MAX_SPELL_SLOTS];

    struct spell* built_in_spells[SPELL_COLUMNS * SPELL_ROWS];

    struct spell custom_spells[MAX_CUSTOM_SPELLS];
};

void inventory_init(struct inventory* inventory);
void inventory_destroy(struct inventory* inventory);

#endif