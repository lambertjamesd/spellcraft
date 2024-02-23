#ifndef __SPELL_SPELL_EXEC_H__
#define __SPELL_SPELL_EXEC_H__

#define MAX_SPELL_EXECUTORS 16

#include "spell.h"

#include "../math/vector3.h"

typedef int spell_slot_id;

struct spell_exec_data {
    union
    {
        struct {
            struct Vector3 pos;
            struct Vector3 vel;
        } projectile;
    } data;
};

struct spell_exec_slot {
    // null when spell isn't active
    struct spell* for_spell;
    uint8_t curr_row;
    uint8_t curr_col;
    struct spell_exec_data data;
    spell_slot_id id;
};

struct spell_exec {
    struct spell_exec_slot slots[MAX_SPELL_EXECUTORS];
    uint8_t next_slot;
};

void spell_exec_start(int slot_index, struct spell* spell);
void spell_exec_stop(int slot_index);

#endif