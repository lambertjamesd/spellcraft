#ifndef __PUSH_SINGLE_TARGET_H__
#define __PUSH_SINGLE_TARGET_H__

#include "../entity/entity_id.h"
#include "../math/vector3.h"
#include "../collision/dynamic_object.h"
#include <stdbool.h>
#include <stdint.h>

struct push_single_definition {
    float acceleration;
    float top_speed;
    float time;
    uint8_t bursty: 1;
    uint8_t lightning: 1;
};

struct push_single_target {
    entity_id target;
    struct push_single_definition* definition;
    struct Vector3 direction;
    float time_left;
};

struct push_single_target* single_push_new(entity_id target, struct Vector3* direction, struct push_single_definition* definiton);
void single_push_apply_velocity_with_dir(struct push_single_definition* definition, struct dynamic_object* obj, struct Vector3* wind_direction);

#endif