#ifndef __SCENE_SCENE_DEFINITION_H__
#define __SCENE_SCENE_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../cutscene/expression.h"
#include <stdint.h>
#include <stdbool.h>

enum entity_type_id {
    ENTITY_TYPE_empty,
    ENTITY_TYPE_biter,
    ENTITY_TYPE_collectable,
    ENTITY_TYPE_crate,
    ENTITY_TYPE_ground_torch,
    ENTITY_TYPE_npc,
    ENTITY_TYPE_training_dummy,
    ENTITY_TYPE_treasure_chest,
    ENTITY_TYPE_water_cube,
    ENTITY_TYPE_mana_plant,
    ENTITY_TYPE_jelly,
    ENTITY_TYPE_jelly_king,
    ENTITY_TYPE_door,
};

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
    SPELL_SYMBOL_BREAK,

    ITEM_TYPE_STAFF_DEFAULT,

    ITEM_TYPE_COUNT,
};

typedef uint32_t collectable_sub_type;

typedef uint16_t room_id;

#define SPELL_SYBMOL_COUNT ITEM_TYPE_STAFF_DEFAULT

struct collectable_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum collectable_type collectable_type;
    collectable_sub_type collectable_sub_type;
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
    bool is_lit;
};

struct water_cube_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    struct Vector3 scale;
};

struct mana_plant_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct jelly_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct jelly_king_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct door_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    room_id room_a;
    room_id room_b;
};

enum npc_type {
    NPC_TYPE_NONE,
    NPC_TYPE_PLAYER,
    NPC_TYPE_SUBJECT,
    NPC_TYPE_MENTOR,
};

enum interaction_type {
    INTERACTION_NONE,

    INTERACTION_LOOK,
    INTERACTION_MOVE,
    INTERACTION_LOOK_MOVE,
    INTERACTION_SPACE,
    INTERACTION_LOOK_SPACE,
    INTERACTION_MOVE_SPACE,
    INTERACTION_LOOK_MOVE_SPACE,

    INTERACTION_WAIT,
    INTERACTION_LOOK_WAIT,
    INTERACTION_MOVE_WAIT,
    INTERACTION_LOOK_MOVE_WAIT,
    INTERACTION_SPACE_WAIT,
    INTERACTION_LOOK_SPACE_WAIT,
    INTERACTION_MOVE_SPACE_WAIT,
    INTERACTION_LOOK_MOVE_SPACE_WAIT,
};

enum fade_colors {
    FADE_COLOR_NONE,
    FADE_COLOR_BLACK,
    FADE_COLOR_WHITE,
};

typedef char* script_location;

struct npc_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum npc_type npc_type;
    script_location dialog;
};

#endif