#ifndef __ENEMIES_JELLY_KING_H__
#define __ENEMIES_JELLY_KING_H__

#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "../render/tmesh.h"
#include "../scene/scene_definition.h"
#include "../render/renderable.h"
#include "../render/animator.h"
#include "jelly.h"

struct jelly_king_animations {
    struct animation_clip* idle;
    struct animation_clip* attack;
    struct animation_clip* attack_ranged;
};

enum jelly_king_state {
    JELLY_KING_IDLE,
    JELLY_KING_MOVE_TO_TARGET,
    JELLY_KING_ATTACK,
    JELLY_KING_ATTACK_RANGED,
    JELLY_KING_ATTACK_AIMING,
};

#define MAX_JELLY_MINIONS   5

struct jelly_king {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct health health;
    struct dynamic_object collider;
    struct spatial_trigger vision;
    struct animation_set* animation_set;
    struct animator animator;
    struct jelly_king_animations animations;
    struct Vector2 max_rotate;
    enum jelly_king_state state;

    union {
        struct {
            int number_left;
        } fire_minion;
    } state_data;

    struct jelly minion[MAX_JELLY_MINIONS];

    uint8_t next_minion;
    uint8_t last_minion;
};

void jelly_king_init(struct jelly_king* jelly_king, struct jelly_king_definition* definition);
void jelly_king_destroy(struct jelly_king* jelly_king);

#endif