#include "live_cast.h"

#include <malloc.h>

#define MAX_COLS    32

void live_cast_init(struct live_cast* live_cast) {
    spell_init(&live_cast->pending_spell, MAX_COLS, 1, 0);
    live_cast->current_spell_output = 0;
    live_cast->active_spells = NULL;
}

void live_cast_destroy(struct live_cast* live_cast) {
    spell_destroy(&live_cast->pending_spell);

    struct active_spell* curr = live_cast->active_spells;

    while (curr) {
        struct active_spell* next = curr->next;
        spell_destroy(&curr->spell);
        free(curr);
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

void live_cast_append_symbol(struct live_cast* live_cast, enum inventory_item_type symbol_type) {
    if (live_cast->current_spell_output >= MAX_COLS) {
        return;
    }

    spell_set_symbol(&live_cast->pending_spell, live_cast->current_spell_output, 0, (struct spell_symbol){
        .type = symbol_type,
    });
    spell_set_symbol(&live_cast->pending_spell, live_cast->current_spell_output + 1, 0, (struct spell_symbol){});
    live_cast->current_spell_output += 1;
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
            spell_destroy(&curr->spell);
            free(curr);

            curr = next;

            if (prev) {
                prev->next = curr;
            } else {
                live_cast->active_spells = curr;
            }
        }
    }
}