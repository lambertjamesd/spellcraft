#include "push_single_target.h"

#include "../effects/effect_allocator.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include <stddef.h>

void single_push_apply_burst_velocity_with_dir(float top_speed, struct dynamic_object* obj, struct Vector3* wind_direction) {
    struct Vector3 tangent;
    vector3ProjectPlane(&obj->velocity, wind_direction, &tangent);
    vector3AddScaled(&tangent, wind_direction, top_speed, &obj->velocity);
    DYNAMIC_OBJECT_MARK_PUSHED(obj);
}

void single_push_apply_velocity_with_dir(struct push_definition* definition, struct dynamic_object* obj, struct Vector3* wind_direction)  {
    struct Vector3 tangent;
    vector3ProjectPlane(&obj->velocity, wind_direction, &tangent);
    struct Vector3 normal;
    vector3Sub(&obj->velocity, &tangent, &normal);
    struct Vector3 wind_velocity;
    vector3Scale(wind_direction, &wind_velocity, definition->top_speed);
    vector3MoveTowards(&normal, &wind_velocity, definition->acceleration * fixed_time_step, &normal);
    vector3Add(&tangent, &normal, &obj->velocity);

    DYNAMIC_OBJECT_MARK_PUSHED(obj);
    obj->velocity.y -= (GRAVITY_CONSTANT - 0.1f) * fixed_time_step;
}

void single_push_update(void* data) {
    struct push_single_target* push_target = (struct push_single_target*)data;

    push_target->time_left -= fixed_time_step;
    struct dynamic_object* obj = collision_scene_find_object(push_target->target);

    if (push_target->time_left < 0.0f || !obj) {
        update_remove(data);
        return;
    }

    if (push_target->definition->bursty) {
        single_push_apply_burst_velocity_with_dir(push_target->definition->top_speed, obj, &push_target->direction);
    } else {
        single_push_apply_velocity_with_dir(push_target->definition, obj, &push_target->direction);
    }
}

struct push_single_target* single_push(entity_id target, struct Vector3* direction, struct push_definition* definiton) {
    struct push_single_target* result = effect_malloc(sizeof(struct push_single_target));

    if (!result) {
        return NULL;
    }

    update_add(result, single_push_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    result->definition = definiton;
    result->time_left = definiton->time;
    result->direction = *direction;
    result->target = target;

    return result;
}