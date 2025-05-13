#ifndef __PUSH_SINGLE_TARGET_H__
#define __PUSH_SINGLE_TARGET_H__

#include "../entity/entity_id.h"
#include "../math/vector3.h"
#include <stdbool.h>

struct push_definition {
    float acceleration;
    float top_speed;
    float time;
    bool bursty: 1;
};

struct push_single_target {
    entity_id target;
    struct push_definition* definition;
    struct Vector3 direction;
    float time_left;
};

struct push_single_target* single_push(entity_id target, struct Vector3* direction, struct push_definition* definiton);

#endif