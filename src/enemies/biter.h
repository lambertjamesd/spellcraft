#ifndef __ENEMIES_BITER_H__
#define __ENEMIES_BITER_H__

#include "../scene/scene_definition.h"

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../math/transform_single_axis.h"
#include "../entity/health.h"
#include "../render/animator.h"

enum biter_animation {
    BITER_ANIMATION_IDLE,
    BITER_ANIMATION_SEE,
    BITER_ANIMATION_WALK,
    BITER_ANIMATION_REST,
    BITER_ANIMATION_TRIP,
    BITER_ANIMATION_CONFUSED,
    BITER_ANIMATION_BITE,
    BITER_ANIMATION_DIE,

    BITER_ANIMATION_COUNT,
};

enum biter_state {
    BITER_STATE_IDLE,
    BITER_STATE_SEE,
    BITER_STATE_CHASE,
    BITER_STATE_REST,
    BITER_STATE_TRIP,
    BITER_STATE_LOSE_PLAYER,
    BITER_STATE_ATTACK,
    BITER_STATE_DIE,
};

typedef enum biter_state biter_state_t;

struct biter {
    biter_state_t current_state;
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object dynamic_object;
    struct spatial_trigger vision;
    struct health health;

    struct animation_set* animation_set;
    animation_clip_t* animations[BITER_ANIMATION_COUNT];
    struct animator animator;

    entity_id current_target;
    float rest_timer;
};

void biter_init(struct biter* biter, struct biter_definition* definition, entity_id id);
void biter_destroy(struct biter* biter);
void biter_common_init();
void biter_common_destroy();

#endif