#ifndef __SPELL_SPELL_EXEC_H__
#define __SPELL_SPELL_EXEC_H__

#define MAX_SPELL_EXECUTORS     16
#define MAX_SOURCE_MODIFIERS    32
#define MAX_BUTTON_INDEX        8    

#include "spell.h"

#include "../math/vector3.h"
#include "projectile.h"
#include "element_emitter.h"
#include "fire_around.h"
#include "explosion.h"
#include "spell_sources.h"
#include "recast.h"
#include "push.h"
#include "mana_pool.h"
#include "lightning.h"

typedef uint32_t spell_slot_id;

union spell_exec_data {
    struct projectile projectile;
    struct element_emitter element_emitter;
    struct fire_around fire_around;
    struct explosion explosion;
    struct recast recast;
    struct push push;
    struct lightning lightning;
};

enum spell_exec_slot_type {
    SPELL_EXEC_SLOT_TYPE_EMPTY,
    SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER,
    SPELL_EXEC_SLOT_TYPE_FIRE_AROUND,
    SPELL_EXEC_SLOT_TYPE_PROJECTILE,
    SPELL_EXEC_SLOT_TYPE_EXPLOSION,
    SPELL_EXEC_SLOT_TYPE_PUSH,
    SPELL_EXEC_SLOT_TYPE_RECAST,
    SPELL_EXEC_SLOT_TYPE_LIGHTNING,
};

struct spell_exec_slot {
    // null when spell isn't active
    struct spell* for_spell;
    uint8_t curr_col;
    uint8_t curr_row;
    uint8_t button_index;
    enum spell_exec_slot_type type;
    union spell_exec_data data;
};

struct spell_source_modifier {
    struct spell_data_source* source;
    struct spell_data_source* output;
    union spell_source_flags flag_mask;
};

struct spell_exec {
    // 0 when the cooresponding slot isn't active
    spell_slot_id ids[MAX_SPELL_EXECUTORS];
    spell_slot_id modifier_ids[MAX_SOURCE_MODIFIERS];
    struct spell_exec_slot slots[MAX_SPELL_EXECUTORS];
    struct spell_source_modifier modifiers[MAX_SOURCE_MODIFIERS];
    struct spell_sources spell_sources;
    struct recast* pending_recast[MAX_BUTTON_INDEX];
    uint8_t next_slot;
    uint8_t next_modifier;
    spell_slot_id next_id;
    int update_id;
};

void spell_exec_init(struct spell_exec* exec);
void spell_exec_destroy(struct spell_exec* exec);
void spell_exec_start(struct spell_exec* exec, int button_index, struct spell* spell, struct spell_data_source* data_source);
void spell_exec_update(struct spell_exec* exec);
void spell_exec_stop(struct spell_exec* exec, int button_index);
bool spell_exec_charge(struct spell_exec* exec);

float spell_exec_max_mana(struct spell_exec* exec);
float spell_exec_current_mana(struct spell_exec* exec);
float spell_exec_prev_mana(struct spell_exec* exec);

#endif