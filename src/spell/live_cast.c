#include "live_cast.h"

#include <malloc.h>
#include "../time/time.h"
#include "../player/inventory.h"

#define MAX_COLS    32

void live_cast_init(struct live_cast* live_cast) {
    spell_init(&live_cast->pending_spell, 0);
    live_cast->current_spell_output = 0;
    live_cast->was_cast = false;
}

void live_cast_destroy(struct live_cast* live_cast) {
    
}

bool live_cast_has_pending_spell(struct live_cast* live_cast) {
    return spell_get_length(&live_cast->pending_spell) > 0 || live_cast->was_cast;
}

bool live_cast_is_typing(struct live_cast* live_cast) {
    return spell_get_length(&live_cast->pending_spell) > 0;
}

struct spell* live_cast_use_spell(struct live_cast* live_cast) {
    live_cast->was_cast = true;

    if (live_cast_get_current_rune(live_cast).primary_rune == ITEM_TYPE_NONE && live_cast->pending_spell.length > 0) {
        --live_cast->pending_spell.length;
    }

    return &live_cast->pending_spell;
}

rune_pattern_t live_cast_get_current_rune(struct live_cast* live_cast) {
    if (spell_get_length(&live_cast->pending_spell) == 0 || live_cast->was_cast) {
        return (rune_pattern_t){};
    }

    return spell_get_rune_pattern(&live_cast->pending_spell, spell_get_length(&live_cast->pending_spell) - 1);
}

int live_cast_prev_rune_count(struct live_cast* live_cast) {
    return live_cast->was_cast ? spell_get_length(&live_cast->pending_spell) : live_cast->current_spell_output;    
}

rune_pattern_t live_cast_get_rune(struct live_cast* live_cast, int index) {
    return spell_get_rune_pattern(&live_cast->pending_spell, index);
}

bool live_cast_toggle(bool curr_value, bool has_room) {
    return !curr_value && has_room;
}

bool live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type) {
    spell_t* spell = &live_cast->pending_spell;
    if (live_cast->was_cast) {
        spell_init(spell, 0);
        live_cast->was_cast = false;
    }

    if (symbol_type == SPELL_SYMBOL_BREAK) {
        if (spell_get_length(spell) == MAX_SPELL_LENGTH || 
            spell_get_rune_pattern(spell, live_cast->current_spell_output).primary_rune == ITEM_TYPE_NONE) {
            return false;
        }
        spell_append(spell, (rune_pattern_t){});
        ++live_cast->current_spell_output;
        return true;
    }

    rune_pattern_t curr = spell_get_rune_pattern(spell, live_cast->current_spell_output);

    if (curr.primary_rune == ITEM_TYPE_NONE) {
        if (!inventory_get_item_level(symbol_type)) {
            return false;
        }

        if (spell_get_length(spell) == 0) {
            spell_append(spell, (rune_pattern_t){.primary_rune = symbol_type,});
        } else {
            spell_set_rune_pattern(spell, live_cast->current_spell_output, (rune_pattern_t){
                .primary_rune = symbol_type,
            });
        }
        return true;
    } else if (curr.primary_rune == symbol_type) {
        spell_set_rune_pattern(spell, live_cast->current_spell_output, (rune_pattern_t){});
        return false;
    }

    int level = inventory_get_item_level(curr.primary_rune);
    bool has_room = rune_pattern_symbol_count(curr) < level;
    
    switch (symbol_type) {
        case SPELL_SYMBOL_FIRE:
            curr.flaming = live_cast_toggle(curr.flaming, has_room);
            break;
        case SPELL_SYMBOL_ICE:
            curr.icy = live_cast_toggle(curr.icy, has_room);
            break;
        case SPELL_SYMBOL_EARTH:
            curr.earthy = live_cast_toggle(curr.earthy, has_room);
            break;
        case SPELL_SYMBOL_AIR:
            curr.windy = live_cast_toggle(curr.windy, has_room);
            break;
        case SPELL_SYMBOL_LIFE:
            curr.living = live_cast_toggle(curr.living, has_room);
            break;
        default:
            return false;
    }

    spell_set_rune_pattern(spell, live_cast->current_spell_output, curr);

    return true;
}