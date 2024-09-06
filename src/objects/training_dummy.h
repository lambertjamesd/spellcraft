#ifndef __OBJECTS_TRAINING_DUMMY_H__
#define __OBJECTS_TRAINING_DUMMY_H__

#include "../scene/scene_definition.h"

#include "../math/transform.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"
#include "../effects/burning_effect.h"

struct training_dummy {
    struct Transform transform;
    struct renderable renderable;
    struct dynamic_object collision;
    struct health health;
    struct Vector3 angularVelocity;
    struct burning_effect* burning_effect;

    uint16_t shock_timer;
};

void training_dummy_init(struct training_dummy* dummy, struct training_dummy_definition* definition);
void training_dummy_destroy(struct training_dummy* dummy);

#endif