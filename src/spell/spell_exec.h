#ifndef __SPELL_SPELL_EXEC_H__
#define __SPELL_SPELL_EXEC_H__

#define MAX_SPELL_EXECUTORS     16
#define MAX_SOURCE_MODIFIERS    32
#define MAX_BUTTON_INDEX        8    

#include "spell.h"

#include "../math/vector3.h"
#include "element_emitter.h"
#include "mana_pool.h"
#include "projectile.h"
#include "push.h"
#include "jump.h"
#include "recast.h"
#include "shield.h"
#include "spell_sources.h"
#include "living_sprite.h"
#include "heal.h"
#include "teleport.h"
#include "stasis.h"
#include "wind.h"
#include "elemental_sword.h"
#include "lightning_storm.h"

typedef uint32_t spell_slot_id;

union spell_exec_data {
    struct projectile projectile;
    struct shield shield;
    struct element_emitter element_emitter;
    struct recast recast;
    struct push push;
    struct jump jump;
    struct living_sprite living_sprite;
    struct spell_heal heal;
    struct teleport teleport;
    struct stasis stasis;
    struct wind wind;
    struct elemental_sword sword;
    lightning_storm_t lightning_storm; 
};

enum spell_exec_slot_type {
    SPELL_EXEC_SLOT_TYPE_EMPTY,
    SPELL_EXEC_SLOT_TYPE_ELEMENT_EMITTER,
    SPELL_EXEC_SLOT_TYPE_PROJECTILE,
    SPELL_EXEC_SLOT_TYPE_SHEILD,
    SPELL_EXEC_SLOT_TYPE_PUSH,
    SPELL_EXEC_SLOT_TYPE_JUMP,
    SPELL_EXEC_SLOT_TYPE_RECAST,
    SPELL_EXEC_SLOT_TYPE_LIVING_SPRITE,
    SPELL_EXEC_SLOT_TYPE_HEAL,
    SPELL_EXEC_SLOT_TYPE_TELEPORT,
    SPELL_EXEC_SLOT_TYPE_STASIS,
    SPELL_EXEC_SLOT_TYPE_WIND,
    SPELL_EXEC_SLOT_TYPE_SWORD,
    SPELL_EXEC_SLOT_TYPE_LIGHTNING_STORM,
};

struct spell_exec_slot {
    // null when spell isn't active
    struct spell* for_spell;
    uint8_t curr_col;
    uint8_t curr_row;
    uint8_t symbol_count;
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

bool spell_exec_is_used(struct spell_exec* exec, struct spell* spell);

float spell_exec_max_mana(struct spell_exec* exec);
float spell_exec_current_mana(struct spell_exec* exec);
float spell_exec_prev_mana(struct spell_exec* exec);

#endif