#include "dynamic_object.h"

#include "../time/time.h"
#include "../math/minmax.h"
#include <math.h>
#include <stddef.h>

void dynamic_object_init(
    entity_id entity_id,
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    uint16_t collision_layers,
    struct Vector3* position, 
    struct Vector2* rotation
) {
    object->entity_id = entity_id;
    object->type = type;
    object->position = position;
    object->rotation = rotation;
    object->pitch = 0;
    object->velocity = gZeroVec;
    object->center = gZeroVec;
    object->time_scalar = 1.0f;
    object->has_gravity = 1;
    object->is_trigger = 0;
    object->is_fixed = 0;
    object->is_out_of_bounds = 0;
    object->collision_layers = collision_layers;
    object->collision_group = 0;
    object->active_contacts = 0;
    object->scale = 1.0f;
    dynamic_object_recalc_bb(object);
}

void dynamic_object_update(struct dynamic_object* object) {
    if (object->is_trigger | object->is_fixed) {
        return;
    }

    if (object->has_gravity) {
        object->velocity.y += fixed_time_step * object->time_scalar * GRAVITY_CONSTANT;
    }

    vector3AddScaled(object->position, &object->velocity, fixed_time_step * object->time_scalar, object->position);
}

struct contact* dynamic_object_nearest_contact(struct dynamic_object* object) {
    struct contact* nearest_target = NULL;
    struct contact* current = object->active_contacts;
    float distance = 0.0f;

    while (current) {
        float check = vector3DistSqrd(&current->point, object->position);
        if (!nearest_target || check < distance) {
            distance = check;
            nearest_target = current;
        }

        current = current->next;
    }

    return nearest_target;
}

bool dynamic_object_is_touching(struct dynamic_object* object, entity_id id) {
    struct contact* current = object->active_contacts;

    while (current) {
        if (current->other_object == id) {
            return true;
        }
            
        current = current->next;
    }

    return false;
}

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct dynamic_object* object = (struct dynamic_object*)data;

    struct Vector3 rotated_dir;

    if (!object->rotation && !object->pitch) {
        object->type->minkowsi_sum(&object->type->data, direction, output);
        vector3Add(output, object->position, output);
        vector3Add(output, &object->center, output);
        return;
    }

    struct Vector3 pitched_dir;

    if (object->pitch) {
        pitched_dir.x = direction->x;
        pitched_dir.y = direction->y * object->pitch->x - direction->z * object->pitch->y;
        pitched_dir.z = direction->z * object->pitch->x + direction->y * object->pitch->y;
    } else {
        pitched_dir = *direction;
    }

    rotated_dir.x = pitched_dir.x * object->rotation->x - pitched_dir.z * object->rotation->y;
    rotated_dir.y = pitched_dir.y;
    rotated_dir.z = pitched_dir.z * object->rotation->x + pitched_dir.x * object->rotation->y;

    struct Vector3 unrotated_out;
    
    object->type->minkowsi_sum(&object->type->data, &rotated_dir, &unrotated_out);

    struct Vector3 unpitched_out;

    unpitched_out.x = unrotated_out.x * object->rotation->x + unrotated_out.z * object->rotation->y;
    unpitched_out.y = unrotated_out.y;
    unpitched_out.z = unrotated_out.z * object->rotation->x - unrotated_out.x * object->rotation->y;

    if (object->pitch) {
        output->x = unpitched_out.x;
        output->y = unpitched_out.y * object->pitch->x + unpitched_out.z * object->pitch->y;
        output->z = unpitched_out.z * object->pitch->x - unpitched_out.y * object->pitch->y;
    } else {
        *output = unpitched_out;
    }

    vector3Add(output, &object->center, output);

    if (object->scale != 1.0f) {
        vector3Scale(output, output, object->scale);
    }

    vector3Add(output, object->position, output);
}

void dynamic_object_recalc_bb(struct dynamic_object* object) {
    object->type->bounding_box(&object->type->data, object->rotation, &object->bounding_box);
    struct Vector3 offset;
    if (object->scale != 1.0f) {
        vector3Scale(&object->bounding_box.min, &object->bounding_box.min, object->scale);
        vector3Scale(&object->bounding_box.max, &object->bounding_box.max, object->scale);
        vector3AddScaled(object->position, &object->center, object->scale, &offset);
    } else {
        vector3Add(&object->center, object->position, &offset);
    }
    vector3Add(&object->bounding_box.min, &offset, &object->bounding_box.min);
    vector3Add(&object->bounding_box.max, &offset, &object->bounding_box.max);
}

bool dynamic_object_should_slide(float max_stable_slope, float normal_y) {
    return normal_y <= 1.0f - max_stable_slope;
}