#ifndef __SPELL_LIVE_CAST_H__
#define __SPELL_LIVE_CAST_H__

#include <stdbool.h>
#include "spell.h"
#include "spell_exec.h"
#include "../scene/scene_definition.h"
#include "../spell/spell_render.h"

struct active_spell {
    struct spell spell;
    struct active_spell* next;  
};

struct live_cast {
    struct spell pending_spell;
    uint8_t current_spell_output;
    struct active_spell* last_spell;
    struct active_spell* active_spells;
    struct spell_render_animation spell_animation;
};

struct live_cast_block {
    uint8_t primary_rune;
    uint8_t secondary_runes;
    uint8_t length;
};

void live_cast_init(struct live_cast* live_cast);
void live_cast_destroy(struct live_cast* live_cast);

bool live_cast_has_pending_spell(struct live_cast* live_cast);
bool live_cast_is_typing(struct live_cast* live_cast);

struct spell* live_cast_extract_active_spell(struct live_cast* live_cast);

bool live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type);

void live_cast_cleanup_unused_spells(struct live_cast* live_cast, struct spell_exec* spell_exec);

void live_cast_render_preview(struct live_cast* live_cast);

void live_cast_get_current_block(struct live_cast* live_cast, struct live_cast_block* block);
#define LIVE_CAST_BLOCK_HAS_SECONDARY(block, rune)    (((block)->secondary_runes & (1 << (rune))) != 0)

#endif