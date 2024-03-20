#ifndef __ENEMIES_BITER_H__
#define __ENEMIES_BITER_H__

#include "../scene/world_definition.h"

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../entity/health.h"

struct biter {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;
    struct dynamic_object dynamic_object;
    struct health health;
};

void biter_init(struct biter* biter, struct biter_definition* definition);
void biter_destroy(struct biter* biter);

#endif