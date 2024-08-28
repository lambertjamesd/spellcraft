#ifndef __ENEMIES_BITER_H__
#define __ENEMIES_BITER_H__

#include "../scene/scene_definition.h"

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../entity/health.h"
#include "../render/animator.h"

struct biter_animations {
    struct animation_clip* idle;
    struct animation_clip* run;
    struct animation_clip* attack;
};

struct biter {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;
    struct dynamic_object dynamic_object;
    struct dynamic_object vision;
    struct health health;

    struct animation_set* animation_set;
    struct biter_animations animations;
    struct animator animator;

    entity_id current_target;
};

void biter_init(struct biter* biter, struct biter_definition* definition);
void biter_destroy(struct biter* biter);

#endif