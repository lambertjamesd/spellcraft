#include "inventory.h"

#include <memory.h>

void inventory_init(struct inventory* inventory) {
    memset(inventory->spell_slots, 0, sizeof(inventory->spell_slots));

    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_init(&inventory->custom_spells[i], SPELL_MAX_COLS, SPELL_MAX_ROWS);
    }
}

void inventory_destroy(struct inventory* inventory) {
    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_destroy(&inventory->custom_spells[i]);
    }
}