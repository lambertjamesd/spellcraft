#include "dynamic_object.h"

#include "../time/time.h"

void dynamic_object_init(
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    struct Vector3* position, 
    struct Vector2* rotation
) {
    object->type = type;
    object->position = position;
    object->rotation = rotation;
    object->velocity = gZeroVec;
    object->time_scalar = 1.0f;
    object->flags = DYNAMIC_OBJECT_GRAVITY;
    object->active_contacts = 0;
}

void dynamic_object_update(struct dynamic_object* object) {
    vector3AddScaled(object->position, &object->velocity, fixed_time_step * object->time_scalar, object->position);

    if (object->flags & DYNAMIC_OBJECT_GRAVITY) {
        object->velocity.y += fixed_time_step * object->time_scalar * GRAVITY_CONSTANT;
    }
}

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct dynamic_object* object = (struct dynamic_object*)data;

    struct Vector3 rotated_dir;

    if (!object->rotation) {
        object->type->minkowsi_sum(&object->type->data, direction, output);
        vector3Add(output, object->position, output);
        return;
    }

    rotated_dir.x = direction->x * object->rotation->x + direction->z * object->rotation->y;
    rotated_dir.y = direction->y;
    rotated_dir.z = direction->z * object->rotation->x - direction->x * object->rotation->y;

    struct Vector3 unrotated_out;
    
    object->type->minkowsi_sum(&object->type->data, &rotated_dir, &unrotated_out);

    output->x = unrotated_out.x * object->rotation->x - unrotated_out.z * object->rotation->y + object->position->x;
    output->y = unrotated_out.y + object->position->y;
    output->z = unrotated_out.z * object->rotation->x + unrotated_out.x * object->rotation->y + object->position->z;
}

void dynamic_object_box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    output->x = direction->x > 0.0f ? shape_data->box.half_size.x : -shape_data->box.half_size.x;
    output->y = direction->y > 0.0f ? shape_data->box.half_size.y : -shape_data->box.half_size.y;
    output->z = direction->z > 0.0f ? shape_data->box.half_size.z : -shape_data->box.half_size.z;
}