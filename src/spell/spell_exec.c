#include "spell_exec.h"

#include <assert.h>
#include <stdbool.h>

void spell_slot_init(
    struct spell_exec_slot* slot, 
    spell_slot_id id,
    struct spell_data_source* input,
    struct spell_data_source* output,
    struct spell* for_spell,
    uint8_t curr_col,
    uint8_t curr_row
) {
    struct spell_symbol symbol = spell_get_symbol(for_spell, curr_col, curr_row);

    switch ((enum spell_symbol_type)symbol.type) {
        case SPELL_SYMBOL_PROJECTILE:
            projectile_init(&slot->data.projectile, input, output);
            break;
        default:
            break;
    }

    slot->id = id;
    slot->type = symbol.type;
    slot->for_spell = for_spell;
    slot->curr_col = curr_col;
    slot->curr_row = curr_row;
}

void spell_slot_destroy(struct spell_exec_slot* slot) {
    switch (slot->type) {
        case SPELL_SYMBOL_PROJECTILE:
            projectile_destroy(&slot->data.projectile);
            break;
        default:
            break;
    }

    slot->type = SPELL_SYMBOL_BLANK;
}

struct spell_exec_slot* spell_exec_find_slot(struct spell_exec* exec) {
    struct spell_exec_slot* result = &exec->slots[exec->next_slot];

    for (int i = 0; i < MAX_SPELL_EXECUTORS; ++i) {
        struct spell_exec_slot* current = &exec->slots[exec->next_slot];

        exec->next_slot += 1;

        if (exec->next_slot == MAX_SPELL_EXECUTORS) {
            exec->next_slot = 0;
        }

        if (current->type == SPELL_SYMBOL_BLANK) {
            return result;
        }

        // select the oldest result if none is available
        if (current->id < result->id) {
            result = current;
        }
    }

    // reuse the oldest active exec
    spell_slot_destroy(result);

    return result;
}

struct spell_data_source* spell_exec_find_source(struct spell_exec* exec) {
    for (int i = 0; i < MAX_SPELL_DATA_SOURCES; ++i) {
        struct spell_data_source* current = &exec->data_sources[exec->next_data_source];

        exec->next_data_source += 1;

        if (exec->next_data_source == MAX_SPELL_DATA_SOURCES) {
            exec->next_data_source = 0;
        }

        if (current->reference_count == 0) {
            return current;
        }
    }

    assert(false);

    return 0;
}

void spell_exec_start(struct spell_exec* exec, int slot_index, struct spell* spell, struct spell_data_source* data_source) {
    struct spell_exec_slot* slot = spell_exec_find_slot(exec);

    spell_slot_id id = exec->next_id;
    exec->next_id += 1;

    spell_slot_init(
        slot,
        id,
        data_source,
        spell_exec_find_source(exec),
        spell,
        0,
        0
    );
}