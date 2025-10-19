#ifndef __SPELL_LIVE_CAST_H__
#define __SPELL_LIVE_CAST_H__

#include <stdbool.h>
#include "spell.h"
#include "spell_exec.h"
#include "../scene/scene_definition.h"
#include "../spell/spell_render.h"

struct live_cast {
    struct spell pending_spell;
    uint8_t current_spell_output;
    uint8_t was_cast;
};

typedef struct live_cast live_cast_t;

void live_cast_init(struct live_cast* live_cast);
void live_cast_destroy(struct live_cast* live_cast);

bool live_cast_has_pending_spell(struct live_cast* live_cast);
bool live_cast_is_typing(struct live_cast* live_cast);

struct spell* live_cast_get_spell(struct live_cast* live_cast);

bool live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type);

rune_pattern_t live_cast_get_current_rune(struct live_cast* live_cast);

#endif