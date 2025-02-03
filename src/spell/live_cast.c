#include "live_cast.h"

#include <malloc.h>
#include "../time/time.h"

#define MAX_COLS    32

void live_cast_init(struct live_cast* live_cast) {
    spell_init(&live_cast->pending_spell, MAX_COLS, 1, 0);
    live_cast->current_spell_output = 0;
    live_cast->active_spells = NULL;
    live_cast->spell_animation.last_symbol_time = 0.0f;
}

void active_spell_free(struct active_spell* active_spell) {
    spell_destroy(&active_spell->spell);
    free(active_spell);
}

void live_cast_destroy(struct live_cast* live_cast) {
    spell_destroy(&live_cast->pending_spell);

    struct active_spell* curr = live_cast->active_spells;

    while (curr) {
        struct active_spell* next = curr->next;
        active_spell_free(curr);
        curr = next;
    }
    live_cast->active_spells = NULL;
}

bool live_cast_has_pending_spell(struct live_cast* live_cast) {
    return live_cast->current_spell_output > 0;
}

struct spell* live_cast_extract_active_spell(struct live_cast* live_cast) {
    if (live_cast->current_spell_output == 0) {
        return NULL;
    }

    struct active_spell* spell = malloc(sizeof(struct active_spell));
    spell_init(&spell->spell, live_cast->current_spell_output, 1, 0);
    memcpy(spell->spell.symbols, live_cast->pending_spell.symbols, live_cast->current_spell_output * sizeof(struct spell_symbol));
    spell->next = live_cast->active_spells;
    live_cast->active_spells = spell;

    spell_set_symbol(&live_cast->pending_spell, 0, 0, (struct spell_symbol){});
    live_cast->current_spell_output = 0;

    return &spell->spell;
}

bool live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type) {
    if (live_cast->current_spell_output >= MAX_COLS) {
        return false;
    }

    if (live_cast->current_spell_output > 1 && 
        symbol_type == SPELL_SYMBOL_BREAK &&
        spell_get_symbol(&live_cast->pending_spell, live_cast->current_spell_output - 1, 0).type == SPELL_SYMBOL_BREAK &&
        spell_get_symbol(&live_cast->pending_spell, live_cast->current_spell_output - 2, 0).type == SPELL_SYMBOL_BREAK) {
        return false;
    }

    for (int prev = live_cast->current_spell_output - 1; prev >= 0; prev -= 1) {
        struct spell_symbol curr_symbol = spell_get_symbol(&live_cast->pending_spell, prev, 0);
        struct spell_symbol prev_symbol = spell_get_symbol(&live_cast->pending_spell, prev - 1, 0);

        if (curr_symbol.type == symbol_type) {
            if (spell_is_rune(symbol_type) && spell_is_rune(prev_symbol.type)) {
                // modifier already applied
                return false;
            }
        } else if (curr_symbol.type == SPELL_SYMBOL_BREAK) {
            break;
        }
    }

    spell_set_symbol(&live_cast->pending_spell, live_cast->current_spell_output, 0, (struct spell_symbol){
        .type = symbol_type,
    });
    spell_set_symbol(&live_cast->pending_spell, live_cast->current_spell_output + 1, 0, (struct spell_symbol){});
    live_cast->current_spell_output += 1;
    live_cast->spell_animation.last_symbol_time = game_time;
    return true;
}

void live_cast_cleanup_unused_spells(struct live_cast* live_cast, struct spell_exec* spell_exec) {
    struct active_spell* prev = NULL;
    struct active_spell* curr = live_cast->active_spells;

    while (curr) {
        if (spell_exec_is_used(spell_exec, &curr->spell)) {
            prev = curr;
            curr = curr->next;
        } else {
            struct active_spell* next = curr->next;
            active_spell_free(curr);

            curr = next;

            if (prev) {
                prev->next = curr;
            } else {
                live_cast->active_spells = curr;
            }
        }
    }
}

void live_cast_render_preview(struct live_cast* live_cast) {
    spell_render(&live_cast->pending_spell, 30, 30, &live_cast->spell_animation);
}