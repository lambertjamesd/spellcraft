#ifndef __SPELL_LIVE_CAST_H__
#define __SPELL_LIVE_CAST_H__

#include <stdbool.h>
#include "spell.h"
#include "spell_exec.h"
#include "../scene/scene_definition.h"

struct active_spell {
    struct spell spell;
    struct active_spell* next;  
};

struct live_cast {
    struct spell pending_spell;
    uint8_t current_spell_output;
    struct active_spell* active_spells;
};

void live_cast_init(struct live_cast* live_cast);
void live_cast_destroy(struct live_cast* live_cast);

bool live_cast_has_pending_spell(struct live_cast* live_cast);

struct spell* live_cast_extract_active_spell(struct live_cast* live_cast);

bool live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type);

void live_cast_cleanup_unused_spells(struct live_cast* live_cast, struct spell_exec* spell_exec);

#endif