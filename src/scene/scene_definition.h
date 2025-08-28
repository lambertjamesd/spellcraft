#ifndef __SCENE_SCENE_DEFINITION_H__
#define __SCENE_SCENE_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/quaternion.h"
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
    ENTITY_TYPE_timed_torch_puzzle,
    ENTITY_TYPE_elevator,
    ENTITY_TYPE_room_portal,
    ENTITY_TYPE_burning_thorns,
    ENTITY_TYPE_bool_and_logic,
    ENTITY_TYPE_camera_focus,
    ENTITY_TYPE_sign
};

struct crate_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    float scale;
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

#define ROOM_NONE 0xFFFF

typedef uint16_t room_id;

#define SPELL_SYBMOL_COUNT ITEM_TYPE_STAFF_DEFAULT

#define VARIABLE_DISCONNECTED   0xFFFF
#define SCENE_VARIABLE_FLAG 0x8000

typedef uint16_t boolean_variable;

typedef char* script_location;

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

enum ground_torch_type {
    GROUND_TORCH_FIRE,
    GROUND_TORCH_LIGHTNING,
};

struct ground_torch_definition {
    struct Vector3 position;
    enum ground_torch_type torch_type;
    boolean_variable lit_source;
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
    boolean_variable unlocked;
};

struct room_portal_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    float scale;
    room_id room_a;
    room_id room_b;
};

struct burning_thorns_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    float scale;
};

struct timed_torch_puzzle_definition {
    struct Vector3 position;
    float torch_time;
    boolean_variable output;
    boolean_variable input_0;
    boolean_variable input_1;
    boolean_variable input_2;
    boolean_variable input_3;
    boolean_variable input_4;
    boolean_variable input_5;
    boolean_variable input_6;
};

struct elevator_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    struct Vector3 target;

    boolean_variable enabled;
    bool inv_enabled;
};

struct bool_and_logic_definition {
    struct Vector3 position;
    boolean_variable output;
    boolean_variable input_0;
    boolean_variable input_1;
    boolean_variable input_2;
    boolean_variable input_3;
    boolean_variable input_4;
    boolean_variable input_5;
    boolean_variable input_6;
    bool should_unset;
    bool input_0_invert;
    bool input_1_invert;
    bool input_2_invert;
    bool input_3_invert;
    bool input_4_invert;
    bool input_5_invert;
    bool input_6_invert;
};

struct camera_focus_definition {
    struct Vector3 position;
    struct Quaternion rotation;
    float fov;
    boolean_variable input;
    boolean_variable output;
    bool repeat;
};

struct sign_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    script_location message;
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
};

enum fade_colors {
    FADE_COLOR_NONE,
    FADE_COLOR_BLACK,
    FADE_COLOR_WHITE,
};

struct npc_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum npc_type npc_type;
    script_location dialog;
};

#endif