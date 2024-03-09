#ifndef __PLAYER_INVENTORY_H__
#define __PLAYER_INVENTORY_H__

#include "../spell/spell.h"

#define MAX_SPELL_SLOTS 4

#define MAX_CUSTOM_SPELLS   8

struct inventory {
    struct spell* spell_slots[MAX_SPELL_SLOTS];

    struct spell custom_spells[MAX_CUSTOM_SPELLS];
};

void inventory_init(struct inventory* inventory);
void inventory_destroy(struct inventory* inventory);

#endif