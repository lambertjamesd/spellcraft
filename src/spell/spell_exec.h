#ifndef __SPELL_SPELL_EXEC_H__
#define __SPELL_SPELL_EXEC_H__

#define MAX_SPELL_EXECUTORS 16

#include "spell.h"

#include "../math/vector3.h"
#include "projectile.h"
#include "spell_data_source.h"

typedef int spell_slot_id;

union spell_exec_data {
    struct projectile projectile;
};

struct spell_exec_slot {
    // null when spell isn't active
    struct spell* for_spell;
    uint8_t curr_col;
    uint8_t curr_row;
    enum spell_symbol_type type;
    union spell_exec_data data;
    spell_slot_id id;
};

struct spell_exec {
    struct spell_exec_slot slots[MAX_SPELL_EXECUTORS];
    struct spell_data_source_pool data_sources;
    uint8_t next_slot;
    spell_slot_id next_id;
};

void spell_exec_start(struct spell_exec* exec, int slot_index, struct spell* spell, struct spell_data_source* data_source);
void spell_exec_stop(struct spell_exec* exec, int slot_index);

#endif