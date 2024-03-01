#include "spell_exec.h"

#include <assert.h>
#include <stdbool.h>
#include <memory.h>
#include "../util/sort.h"
#include "../time/time.h"

void spell_exec_step(struct spell_exec* exec, int button_index, struct spell* spell, int col, int row, struct spell_data_source* data_source);

void spell_slot_init(
    struct spell_exec_slot* slot, 
    int button_index,
    struct spell_data_source* input,
    struct spell* for_spell,
    uint8_t curr_col,
    uint8_t curr_row
) {
    struct spell_symbol symbol = spell_get_symbol(for_spell, curr_col, curr_row);

    struct spell_event_options event_options;

    event_options.has_primary_event = spell_has_primary_event(for_spell, curr_col, curr_row);
    event_options.has_secondary_event = spell_has_secondary_event(for_spell, curr_col, curr_row);

    switch ((enum spell_symbol_type)symbol.type) {
        case SPELL_SYMBOL_PROJECTILE:
            slot->type = SPELL_EXEC_SLOT_TYPE_PROJECTILE;
            projectile_init(&slot->data.projectile, input, event_options);
            break;
        case SPELL_SYMBOL_FIRE:
            if (input->flags.cast_state != SPELL_CAST_STATE_INSTANT) {
                slot->type = SPELL_EXEC_SLOT_TYPE_FIRE;
                fire_init(&slot->data.fire, input, event_options);
            } else {
                slot->type = SPELL_EXEC_SLOT_TYPE_EXPLOSION;
                explosion_init(&slot->data.explosion, input, event_options);
            }
            break;
        case SPELL_SYBMOL_RECAST:
            slot->type = SPELL_EXEC_SLOT_TYPE_RECAST;
            recast_init(&slot->data.recast, input, event_options);
            break;
        default:
            assert(false);
            slot->type = SPELL_EXEC_SLOT_TYPE_EMPTY;
            break;
    }

    slot->button_index = button_index;
    slot->for_spell = for_spell;
    slot->curr_col = curr_col;
    slot->curr_row = curr_row;
}

void spell_slot_destroy(struct spell_exec* exec, int slot_index) {
    struct spell_exec_slot* slot = &exec->slots[slot_index];

    struct recast* remove_recast = NULL;

    switch (slot->type) {
        case SPELL_EXEC_SLOT_TYPE_PROJECTILE:
            projectile_destroy(&slot->data.projectile);
            break;
        case SPELL_EXEC_SLOT_TYPE_FIRE:
            fire_destroy(&slot->data.fire);
            break;
        case SPELL_EXEC_SLOT_TYPE_EXPLOSION:
            explosion_destroy(&slot->data.explosion);
            break;
        case SPELL_EXEC_SLOT_TYPE_RECAST:
            recast_destroy(&slot->data.recast);
            remove_recast = &slot->data.recast;
            break;
        default:
            break;
    }

    if (remove_recast) {
        struct recast* prev = NULL;
        struct recast* curr = exec->pending_recast[slot->button_index];

        while (curr) {
            if (curr == remove_recast) {
                if (prev) {
                    prev->next_recast = curr->next_recast;
                } else {
                    exec->pending_recast[slot->button_index] = curr->next_recast;
                }
                break;
            }

            prev = curr;
            curr = curr->next_recast;
        }
    }

    exec->ids[slot_index] = 0;
}

void spell_slot_update(struct spell_exec* exec, int spell_slot_index) {
    struct spell_exec_slot* slot = &exec->slots[spell_slot_index];

    struct spell_event_listener event_listener;
    spell_event_listener_init(&event_listener);

    switch (slot->type) {
        case SPELL_EXEC_SLOT_TYPE_PROJECTILE:
            projectile_update(&slot->data.projectile, &event_listener, &exec->data_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_FIRE:
            fire_update(&slot->data.fire, &event_listener, &exec->data_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_EXPLOSION:
            explosion_update(&slot->data.explosion, &event_listener, &exec->data_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_RECAST:
            recast_update(&slot->data.recast, &event_listener, &exec->data_sources);
            break;
        default:
            break;
    }

    // handle destroy event first 
    for (int i = 0; i < event_listener.event_count; ++i) {
        if (event_listener.events[i].type == SPELL_EVENT_DESTROY) {
            spell_slot_destroy(exec, spell_slot_index);
            exec->ids[spell_slot_index] = 0;
            break;
        }
    }

    // then handle the remaining events
    for (int i = 0; i < event_listener.event_count; ++i) {
        switch (event_listener.events[i].type) {
        case SPELL_EVENT_PRIMARY:
            if (spell_has_primary_event(slot->for_spell, slot->curr_col, slot->curr_row) != SPELL_SYMBOL_BLANK) {
                spell_exec_step(exec, slot->button_index, slot->for_spell, slot->curr_col + 1, slot->curr_row, event_listener.events[i].data_source);
            }
            break;
        case SPELL_EVENT_SECONDARY:
            if (spell_has_secondary_event(slot->for_spell, slot->curr_col, slot->curr_row))  {
                spell_exec_step(exec, slot->button_index, slot->for_spell, slot->curr_col + 1, slot->curr_row + 1, event_listener.events[i].data_source);
            }
            break;
        }
    }

    spell_event_listener_destroy(&event_listener);
}

int spell_exec_find_slot(struct spell_exec* exec) {
    int result = 0;
    int result_id = exec->ids[0];

    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        int id = exec->ids[exec->next_slot];
        int current = exec->next_slot;

        exec->next_slot += 1;

        if (exec->next_slot == MAX_SPELL_EXECUTORS) {
            exec->next_slot = 0;
        }

        if (id == 0) {
            return current;
        }

        // select the oldest result if none is available
        if (id < result_id) {
            result = current;
            result_id = id;
        }
    }

    // reuse the oldest active slot
    spell_slot_destroy(exec, result);

    return result;
}

void spell_exec_step(struct spell_exec* exec, int button_index, struct spell* spell, int col, int row, struct spell_data_source* data_source) {
    if (!data_source) {
        return;
    }

    int slot_index = spell_exec_find_slot(exec);

    spell_slot_id id = exec->next_id;
    exec->next_id += 1;

    exec->ids[slot_index] = id;

    struct spell_exec_slot* slot = &exec->slots[slot_index];

    spell_slot_init(
        slot,
        button_index,
        data_source,
        spell,
        col,
        row
    );

    if (slot->type == SPELL_EXEC_SLOT_TYPE_RECAST) {
        slot->data.recast.next_recast = exec->pending_recast[button_index];
        exec->pending_recast[button_index] = &slot->data.recast;
    }
}

void spell_exec_init(struct spell_exec* exec) {
    exec->next_id = 1;
    exec->next_slot = 0;
    spell_data_source_pool_init(&exec->data_sources);
    memset(&exec->ids, 0, sizeof(exec->ids));
    exec->update_id = update_add(exec, (update_callback)spell_exec_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);
    memset(exec->pending_recast, 0, sizeof(exec->pending_recast));
}

void spell_exec_destroy(struct spell_exec* exec) {
    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        if (exec->ids[i]) {
            spell_slot_destroy(exec, i);
        }
    }
    update_remove(exec->update_id);
}

void spell_exec_recast(struct spell_exec* exec, int button_index, struct spell_data_source* data_source) {
    struct recast* recast = exec->pending_recast[button_index];

    while (recast) {
        recast_recast(recast, data_source);     
        struct recast* next = recast->next_recast;
        recast->next_recast = 0;   
        recast = next;
    }

    exec->pending_recast[button_index] = 0;
}

void spell_exec_start(struct spell_exec* exec, int button_index, struct spell* spell, struct spell_data_source* data_source) {
    if (exec->pending_recast[button_index]) {
        spell_exec_recast(exec, button_index, data_source);
        return;
    }

    spell_exec_step(exec, button_index, spell, 0, 0, data_source);
}

int spell_slot_compare(spell_slot_id* ids, uint16_t a, uint16_t b) {
    return (int)ids[a] - (int)ids[b];
}

void spell_exec_update(struct spell_exec* exec) {
    uint16_t indices[MAX_SPELL_EXECUTORS];
    int count = 0;

    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        if (exec->ids[i]) {
            indices[count] = i;
            count += 1;
        }
    }

    // update from oldest spell to newest
    sort_indices(indices, count, exec->ids, (sort_compare)spell_slot_compare);

    for (int i = 0; i < count; ++i) {
        spell_slot_update(exec, indices[i]);
    }
}