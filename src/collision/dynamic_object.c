#include "dynamic_object.h"

#include "../time/time.h"

void dynamic_object_update(struct dynamic_object* object) {
    vector3AddScaled(object->position, &object->velocity, fixed_time_step * object->time_scalar, object->position);
    object->velocity.y += fixed_time_step * object->time_scalar * GRAVITY_CONSTANT;
}

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct dynamic_object* object = (struct dynamic_object*)data;

    struct Vector3 rotated_dir;

    rotated_dir.x = direction->x * object->rotation->x + direction->z * object->rotation->y;
    rotated_dir.y = direction->y;
    rotated_dir.z = direction->z * object->rotation->x - direction->x * object->rotation->y;

    struct Vector3 unrotated_out;
    
    object->type->minkowsi_sum(&object->type->data, &rotated_dir, &unrotated_out);

    output->x = unrotated_out.x * object->rotation->x - unrotated_out.z * object->rotation->y + object->position->x;
    output->y = unrotated_out.y + object->position->y;
    output->z = unrotated_out.z * object->rotation->x + unrotated_out.x * object->rotation->y + object->position->z;
}