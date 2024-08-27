#ifndef __SCENE_WORLD_DEFINITION_H__
#define __SCENE_WORLD_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../cutscene/expression.h"
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

enum inventory_item_type {
    ITEM_TYPE_NONE,
    SPELL_SYMBOL_FIRE,
    SPELL_SYMBOL_ICE,
    SPELL_SYMBOL_EARTH,
    SPELL_SYMBOL_AIR,
    SPELL_SYMBOL_LIFE,

    SPELL_SYMBOL_RECAST,
    SPELL_SYMBOL_PASS_DOWN,

    ITEM_TYPE_STAFF_DEFAULT,

    ITEM_TYPE_COUNT,
};

#define SPELL_SYBMOL_COUNT ITEM_TYPE_STAFF_DEFAULT

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

struct treasure_chest_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum inventory_item_type item;
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