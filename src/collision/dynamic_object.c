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
    object->collision_layers = collision_layers;
    object->active_contacts = 0;
    dynamic_object_recalc_bb(object);
}

void dynamic_object_update(struct dynamic_object* object) {
    if (object->is_trigger | object->is_fixed) {
        return;
    }

    vector3AddScaled(object->position, &object->velocity, fixed_time_step * object->time_scalar, object->position);

    if (object->has_gravity) {
        object->velocity.y += fixed_time_step * object->time_scalar * GRAVITY_CONSTANT;
    }
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

    rotated_dir.x = pitched_dir.x * object->rotation->x + pitched_dir.z * object->rotation->y;
    rotated_dir.y = pitched_dir.y;
    rotated_dir.z = pitched_dir.z * object->rotation->x - pitched_dir.x * object->rotation->y;

    struct Vector3 unrotated_out;
    
    object->type->minkowsi_sum(&object->type->data, &rotated_dir, &unrotated_out);

    struct Vector3 unpitched_out;

    unpitched_out.x = unrotated_out.x * object->rotation->x - unrotated_out.z * object->rotation->y + object->position->x;
    unpitched_out.y = unrotated_out.y + object->position->y;
    unpitched_out.z = unrotated_out.z * object->rotation->x + unrotated_out.x * object->rotation->y + object->position->z;

    if (object->pitch) {
        output->x = unpitched_out.x;
        output->y = unpitched_out.y * object->pitch->x + unpitched_out.z * object->pitch->y;
        output->z = unpitched_out.z * object->pitch->x - unpitched_out.y * object->pitch->y;
    } else {
        *output = unpitched_out;
    }

    vector3Add(output, &object->center, output);
}

void dynamic_object_recalc_bb(struct dynamic_object* object) {
    object->type->bounding_box(&object->type->data, object->rotation, &object->bounding_box);
    vector3Add(&object->bounding_box.min, object->position, &object->bounding_box.min);
    vector3Add(&object->bounding_box.max, object->position, &object->bounding_box.max);
}

void dynamic_object_box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    output->x = direction->x > 0.0f ? shape_data->box.half_size.x : -shape_data->box.half_size.x;
    output->y = direction->y > 0.0f ? shape_data->box.half_size.y : -shape_data->box.half_size.y;
    output->z = direction->z > 0.0f ? shape_data->box.half_size.z : -shape_data->box.half_size.z;
}

void dynamic_object_box_bouding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    struct Vector3* half_size = &shape_data->box.half_size;

    if (!rotation) {
        vector3Negate(half_size, &box->min);
        box->max = *half_size;
        return;
    }

    box->max.x = half_size->x * fabsf(rotation->x) + half_size->z * fabsf(rotation->y);
    box->max.y = half_size->y;
    box->max.z = half_size->x * fabsf(rotation->y) + half_size->z * fabsf(rotation->x);

    vector3Negate(&box->max, &box->min);
}

void dynamic_object_cone_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    output->x = direction->x > 0.0f ? shape_data->cone.size.x : -shape_data->cone.size.x;
    output->y = direction->y > 0.0f ? shape_data->cone.size.y : -shape_data->cone.size.y;
    output->z = shape_data->cone.size.z;

    if (vector3Dot(output, direction) < 0) {
        *output = gZeroVec;
    }
}

void dynamic_object_cone_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    struct Vector2 corner;

    corner.x = shape_data->cone.size.x;
    corner.y = shape_data->cone.size.z;

    struct Vector2 cornerRotated;
    vector2ComplexMul(&corner, rotation, &cornerRotated);

    box->min.x = cornerRotated.x;
    box->min.y = -shape_data->cone.size.y;
    box->min.z = cornerRotated.y;

    box->max = box->min;
    box->max.y = shape_data->cone.size.y;

    corner.x = -corner.x;
    vector2ComplexMul(&corner, rotation, &cornerRotated);

    box->min.x = MIN(box->min.x, cornerRotated.x);
    box->min.z = MIN(box->min.z, cornerRotated.y);

    box->max.x = MAX(box->max.x, cornerRotated.x);
    box->max.z = MAX(box->max.z, cornerRotated.y);

    box->min.x = MIN(box->min.x, 0.0f);
    box->min.z = MIN(box->min.z, 0.0f);

    box->max.x = MAX(box->max.x, 0.0f);
    box->max.z = MAX(box->max.z, 0.0f);
}

#define SQRT_1_3  0.577350269f

void dynamic_object_sphere_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float radius = shape_data->sphere.radius;

    float distance = fabsf(direction->x);
    output->x = direction->x > 0.0f ? radius : -radius;
    output->y = 0.0f;
    output->z = 0.0f;

    for (int i = 1; i < 3; ++i) {
        float distanceCheck = fabsf(VECTOR3_AS_ARRAY(direction)[i]);

        if (distanceCheck > distance) {
            distance = distanceCheck;
            *output = gZeroVec;
            if (VECTOR3_AS_ARRAY(direction)[i] > 0.0f) {
                VECTOR3_AS_ARRAY(output)[i] = radius;
            } else {
                VECTOR3_AS_ARRAY(output)[i] = -radius;
            }
        }
    }

    // float distanceCheck = (fabsf(direction->x) + fabsf(direction->y) + fabsf(direction->z)) * SQRT_1_3;

    // if (distanceCheck > distance) {
    //     float scaledRadius = radius * SQRT_1_3;

    //     if (output->x > 0.0f) {
    //         output->x = scaledRadius;
    //     } else {
    //         output->x = -scaledRadius;
    //     }

    //     if (output->y > 0.0f) {
    //         output->y = scaledRadius;
    //     } else {
    //         output->y = -scaledRadius;
    //     }

    //     if (output->z > 0.0f) {
    //         output->z = scaledRadius;
    //     } else {
    //         output->z = -scaledRadius;
    //     }
    // }
}

void dynamic_object_sphere_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    vector3Scale(&gOneVec, &box->max, shape_data->sphere.radius);
    vector3Scale(&gOneVec, &box->min, -shape_data->sphere.radius);
}

void dynamic_object_capsule_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;
    dynamic_object_sphere_minkowski_sum(data, direction, output);

    if (direction->y > 0.0f) {
        output->y += shape_data->capsule.inner_half_height;
    } else {
        output->y -= shape_data->capsule.inner_half_height;
    }
}

void dynamic_object_capsule_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    vector3Scale(&gOneVec, &box->max, shape_data->capsule.radius);
    vector3Scale(&gOneVec, &box->min, -shape_data->capsule.radius);

    box->max.y += shape_data->capsule.inner_half_height;
    box->min.y -= shape_data->capsule.inner_half_height;
}

#define SQRT_1_2   0.707106781f

void dynamic_object_cylinder_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float abs_x = fabsf(direction->x);
    float abs_y = fabsf(direction->y);
    float angle_dot = (abs_x + abs_y) * SQRT_1_2;
    bool use_angle = false;

    if (abs_x > angle_dot && abs_y > angle_dot) {
        output->x = direction->x > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
        output->z = direction->z > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
    } else if (abs_x > abs_y) {
        output->x = direction->x > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
        output->z = 0.0f;
    } else {
        output->x = 0.0f;
        output->z = direction->z > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
    }

    output->y = direction->y > 0.0f ? shape_data->cylinder.half_height : -shape_data->cylinder.half_height;
}

void dynamic_object_cylinder_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    box->min.x = -shape_data->cylinder.radius;
    box->min.y = -shape_data->cylinder.half_height;
    box->min.z = -shape_data->cylinder.radius;

    box->max.x = shape_data->cylinder.radius;
    box->max.y = shape_data->cylinder.half_height;
    box->max.z = shape_data->cylinder.radius;
}