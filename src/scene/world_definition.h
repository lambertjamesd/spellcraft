#ifndef __SCENE_WORLD_DEFINITION_H__
#define __SCENE_WORLD_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include <stdint.h>

struct crate_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

enum collectable_type {
    COLLECTABLE_TYPE_HEALTH,
    COLLECTABLE_TYPE_POTION,
    COLLECTABLE_TYPE_SPELL_RUNE,
};

enum spell_symbol_type {
    SPELL_SYMBOL_BLANK,
    SPELL_SYMBOL_FIRE,
    SPELL_SYMBOL_PROJECTILE,
    SPELL_SYMBOL_PUSH,
    SPELL_SYMBOL_RECAST,
    SPELL_SYMBOL_SHIELD,
    SPELL_SYMBOL_REVERSE,
    SPELL_SYMBOL_TARGET,
    SPELL_SYMBOL_UP,
    SPELL_SYMBOL_TIME_DIALATION,
    
    SPELL_SYMBOL_PASS_DOWN,

    SPELL_SYBMOL_COUNT,
};

struct collectable_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum collectable_type collectable_type;
    uint32_t collectable_sub_type;
};

struct training_dummy_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct biter_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct ground_torch_definition {
    struct Vector3 position;
    uint32_t is_lit;
};

enum npc_type {
    NPC_TYPE_MENTOR,
};

struct npc_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum npc_type npc_type;
    char* dialog;
};

#endif