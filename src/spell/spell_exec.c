#include "spell_exec.h"

#include <assert.h>
#include <stdbool.h>
#include <memory.h>
#include "../util/sort.h"
#include "../time/time.h"
#include "elements.h"
#include "element_emitter_definitions.h"
#include "living_sprite_definitions.h"

void spell_exec_step(struct spell_exec* exec, int button_index, struct spell* spell, int col, int row, struct spell_data_source* data_source, float burst_mana);

void spell_slot_init(
    struct spell_exec_slot* slot, 
    int button_index,
    struct spell_data_source* input,
    union spell_modifier_flags modifier_flags,
    float burst_mana,
    struct spell* for_spell,
    uint8_t curr_col,
    uint8_t curr_row
) {
    struct spell_symbol symbol = spell_get_symbol(for_spell, curr_col, curr_row);

    struct spell_event_options event_options;

    event_options.has_primary_event = spell_has_primary_event(for_spell, curr_col, curr_row);
    event_options.has_secondary_event = spell_has_secondary_event(for_spell, curr_col, curr_row);
    event_options.burst_mana = burst_mana;

    switch ((enum inventory_item_type)symbol.type) {
        case SPELL_SYMBOL_EARTH:
            if (modifier_flags.windy) {
                slot->type = SPELL_EXEC_SLOT_TYPE_PROJECTILE;
                projectile_init(&slot->data.projectile, input, modifier_flags, event_options, spell_data_source_determine_element(modifier_flags));
            } else {
                slot->type = SPELL_EXEC_SLOT_TYPE_SHEILD;
                shield_init(&slot->data.shield, input, event_options, spell_data_source_determine_element(modifier_flags));
            }
            break;
        case SPELL_SYMBOL_FIRE:
            if (modifier_flags.living) {
                struct living_sprite_definition* def = living_sprite_find_def(ELEMENT_TYPE_FIRE, modifier_flags.windy, modifier_flags.flaming, modifier_flags.icy);
                slot->type = SPELL_EXEC_SLOT_TYPE_LIVING_SPRITE;
                living_sprite_init(&slot->data.living_sprite, input, event_options, def);
            } else if (input->flags.cast_state == SPELL_CAST_STATE_INSTANT) {
                slot->type = SPELL_EXEC_SLOT_TYPE_EXPLOSION;
                explosion_init(&slot->data.explosion, input, event_options, modifier_flags.icy ? ELEMENT_TYPE_LIGHTNING : ELEMENT_TYPE_FIRE);
            } else {
                struct element_emitter_definition* def = element_emitter_find_def(ELEMENT_TYPE_FIRE, modifier_flags.windy, modifier_flags.flaming, modifier_flags.icy);
                slot->type = SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER;
                element_emitter_init(&slot->data.element_emitter, input, event_options, def);
            }
            break;
        case SPELL_SYMBOL_ICE:
            if (modifier_flags.living) {
                struct living_sprite_definition* def = living_sprite_find_def(ELEMENT_TYPE_ICE, modifier_flags.windy, modifier_flags.flaming, modifier_flags.icy);
                slot->type = SPELL_EXEC_SLOT_TYPE_LIVING_SPRITE;
                living_sprite_init(&slot->data.living_sprite, input, event_options, def);
            } else if (input->flags.cast_state == SPELL_CAST_STATE_INSTANT) {
                slot->type = SPELL_EXEC_SLOT_TYPE_EXPLOSION;
                explosion_init(&slot->data.explosion, input, event_options, modifier_flags.flaming ? ELEMENT_TYPE_LIGHTNING : ELEMENT_TYPE_ICE);
            } else {
                struct element_emitter_definition* def = element_emitter_find_def(ELEMENT_TYPE_ICE, modifier_flags.windy, modifier_flags.flaming, modifier_flags.icy);
                slot->type = SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER;
                element_emitter_init(&slot->data.element_emitter, input, event_options, def);
            }
            break;
        case SPELL_SYMBOL_BREAK:
            slot->type = SPELL_EXEC_SLOT_TYPE_RECAST;
            recast_init(&slot->data.recast, input, event_options, RECAST_MODE_RECAST);
            break;
        case SPELL_SYMBOL_AIR:
            slot->type = SPELL_EXEC_SLOT_TYPE_PUSH;
            push_init(&slot->data.push, input, event_options, spell_data_source_determine_element(modifier_flags));
            break;
        case SPELL_SYMBOL_LIFE:
            slot->type = SPELL_EXEC_SLOT_TYPE_HEAL;
            spell_heal_init(&slot->data.heal, input, event_options);
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
        case SPELL_EXEC_SLOT_TYPE_SHEILD:
            shield_destroy(&slot->data.shield);
            break;
        case SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER:
            element_emitter_destroy(&slot->data.element_emitter);
            break;
        case SPELL_EXEC_SLOT_TYPE_EXPLOSION:
            explosion_destroy(&slot->data.explosion);
            break;
        case SPELL_EXEC_SLOT_TYPE_RECAST:
            recast_destroy(&slot->data.recast);
            remove_recast = &slot->data.recast;
            break;
        case SPELL_EXEC_SLOT_TYPE_PUSH:
            push_destroy(&slot->data.push);
            break;
        case SPELL_EXEC_SLOT_TYPE_LIVING_SPRITE:
            living_sprite_destroy(&slot->data.living_sprite);
            break;
        case SPELL_EXEC_SLOT_TYPE_HEAL:
            spell_heal_destroy(&slot->data.heal);
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

    bool isActive = false;

    switch (slot->type) {
        case SPELL_EXEC_SLOT_TYPE_PROJECTILE:
            isActive = projectile_update(&slot->data.projectile, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_SHEILD:
            isActive = shield_update(&slot->data.shield, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER:
            isActive = element_emitter_update(&slot->data.element_emitter, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_EXPLOSION:
            isActive = explosion_update(&slot->data.explosion, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_RECAST:
            isActive = recast_update(&slot->data.recast, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_PUSH:
            isActive = push_update(&slot->data.push, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_LIVING_SPRITE:
            isActive = living_sprite_update(&slot->data.living_sprite, &event_listener, &exec->spell_sources);
            break;
        case SPELL_EXEC_SLOT_TYPE_HEAL:
            isActive = spell_heal_update(&slot->data.heal, &event_listener, &exec->spell_sources);
            break;
        default:
            break;
    }

    if (!isActive) {
        spell_slot_destroy(exec, spell_slot_index);
        exec->ids[spell_slot_index] = 0;
    }

    // then handle the remaining events
    for (int i = 0; i < event_listener.event_count; ++i) {
        struct spell_event* event = &event_listener.events[i];

        switch (event->type) {
        case SPELL_EVENT_PRIMARY:
        {
            int event_start = spell_get_primary_event(slot->for_spell, slot->curr_col, slot->curr_row);
            if (event_start != -1) {
                spell_exec_step(exec, slot->button_index, slot->for_spell, event_start, slot->curr_row, event->data_source, event->burst_mana);
            }
            break;
        }
        case SPELL_EVENT_SECONDARY:
            if (spell_has_secondary_event(slot->for_spell, slot->curr_col, slot->curr_row))  {
                spell_exec_step(exec, slot->button_index, slot->for_spell, slot->curr_col + 1, slot->curr_row + 1, event->data_source, event->burst_mana);
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

void spell_source_modifier_apply(struct spell_source_modifier* modifier) {
    modifier->output->flags.all = modifier->source->flags.all | modifier->flag_mask.all;
    modifier->output->direction = modifier->source->direction;
    modifier->output->position = modifier->source->position;
    modifier->output->target = modifier->source->target;
}

void spell_source_modifier_init(struct spell_source_modifier* modifier, struct spell_data_source* source, struct spell_data_source* output, union spell_source_flags flag_mask) {
    modifier->source = source;
    spell_data_source_retain(source);
    modifier->output = output;
    spell_data_source_retain(output);
    modifier->flag_mask = flag_mask;

    spell_source_modifier_apply(modifier);
}

void spell_source_modifier_destroy(struct spell_exec* exec, int index) {
    struct spell_source_modifier* modifier = &exec->modifiers[index];
    spell_data_source_release(modifier->source);
    spell_data_source_release(modifier->output);
    exec->modifier_ids[index] = 0;
}

int spell_exec_find_modifier(struct spell_exec* exec) {
    int result = 0;
    int result_id = exec->modifier_ids[0];

    for (int i = 0; i < MAX_SOURCE_MODIFIERS; ++i) {
        int id = exec->modifier_ids[exec->next_modifier];
        int current = exec->next_modifier;

        exec->next_modifier += 1;

        if (exec->next_modifier == MAX_SOURCE_MODIFIERS) {
            exec->next_modifier = 0;
        }

        if (id == 0) {
            return current;
        }

        // if there is no consumer of this modifier it can be reused
        if (exec->modifiers[current].output->reference_count == 1) {
            spell_source_modifier_destroy(exec, current);
            return current;
        }

        if (id < result_id) {
            result = current;
            result_id = id;
        }
    }

    spell_source_modifier_destroy(exec, result);

    return result;
}

void spell_exec_step(struct spell_exec* exec, int button_index, struct spell* spell, int col, int row, struct spell_data_source* data_source, float burst_mana) {
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
        spell_get_modifiers(spell, col, row),
        burst_mana,
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
    spell_sources_init(&exec->spell_sources);
    memset(&exec->ids, 0, sizeof(exec->ids));
    memset(&exec->modifier_ids, 0, sizeof(exec->modifier_ids));
    update_add(exec, (update_callback)spell_exec_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);
    memset(exec->pending_recast, 0, sizeof(exec->pending_recast));
}

void spell_exec_destroy(struct spell_exec* exec) {
    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        if (exec->ids[i]) {
            spell_slot_destroy(exec, i);
        }
    }
    update_remove(exec);
}

void spell_exec_recast(struct spell_exec* exec, int button_index, struct spell_data_source* data_source) {
    struct recast* recast = exec->pending_recast[button_index];
    
    int count = 0;

    while (recast) {
        count += 1;
        recast = recast->next_recast;
    }

    recast = exec->pending_recast[button_index];

    float burst_amount = mana_pool_request_charged_mana(&exec->spell_sources.mana_pool) / count;

    while (recast) {
        recast_recast(recast, data_source, burst_amount);     
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

    if (spell_get_symbol(spell, 0, 0).type == SPELL_EXEC_SLOT_TYPE_EMPTY) {
        return;
    }

    spell_exec_step(exec, button_index, spell, 0, 0, data_source, 0.0f);
}

bool spell_exec_charge(struct spell_exec* exec) {
    for (int i = 0; i < MAX_BUTTON_INDEX; i += 1) {
        if (exec->pending_recast[i]) {
            mana_pool_charge(&exec->spell_sources.mana_pool, 10.0f);
            return true;
        }
    }

    return false;
}

bool spell_exec_is_used(struct spell_exec* exec, struct spell* spell) {
    for (int i = 0; i < MAX_SPELL_EXECUTORS; i += 1) {
        if (exec->slots[i].for_spell == spell) {
            return true;
        }
    }

    return true;
}

#define GET_ID_FROM_INDEX(exec, idx) (int)(((idx) & 0x8000) ? (exec)->modifier_ids[(idx) & 0x7FFF] : (exec)->ids[idx])
#define DEFINE_MODIFIER_IDX(idx) ((idx) | 0x8000)
#define IS_MODIFIER_ID(idx) ((idx) & 0x8000)
#define EXTRACT_MODIFIER_IDX(idx) ((idx) & 0x7FFF)

int spell_slot_compare(struct spell_exec* exec, uint16_t a, uint16_t b) {
    return GET_ID_FROM_INDEX(exec, a) - GET_ID_FROM_INDEX(exec, b);
}

void spell_exec_update(struct spell_exec* exec) {
    uint16_t indices[MAX_SPELL_EXECUTORS + MAX_SOURCE_MODIFIERS];
    spell_slot_id start_ids[MAX_SPELL_EXECUTORS];
    spell_slot_id start_modifier_id[MAX_SOURCE_MODIFIERS];
    int count = 0;

    mana_pool_update(&exec->spell_sources.mana_pool);

    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        if (exec->ids[i]) {
            indices[count] = i;
            start_ids[i] = exec->ids[i];
            count += 1;
        }
    }

    for (int i = 0; i < MAX_SOURCE_MODIFIERS; ++i) {
        if (exec->modifier_ids[i]) {
            indices[count] = DEFINE_MODIFIER_IDX(i);
            start_modifier_id[i] = exec->modifier_ids[i];
            count += 1;
        }
    }

    // update from oldest spell to newest
    sort_indices(indices, count, exec, (sort_compare)spell_slot_compare);

    for (int i = 0; i < count; ++i) {
        uint16_t index = indices[i];
        if (IS_MODIFIER_ID(index)) {
            index = EXTRACT_MODIFIER_IDX(index);
            // make sure modifier hasn't been replaced
            if (start_modifier_id[index] == exec->modifier_ids[index]) {
                spell_source_modifier_apply(&exec->modifiers[index]);
            }
        } else {
            // make sur ethe slot hasn'te been replaced
            if (start_ids[index] == exec->ids[index]) {
                spell_slot_update(exec, index);
            }
        }
    }
}

float spell_exec_max_mana(struct spell_exec* exec) {
    return exec->spell_sources.mana_pool.max_mana;
}

float spell_exec_current_mana(struct spell_exec* exec) {
    return exec->spell_sources.mana_pool.current_mana;
}

float spell_exec_prev_mana(struct spell_exec* exec) {
    return mana_pool_get_previous_mana(&exec->spell_sources.mana_pool);
}