#include "spatial_trigger.h"
#include "shapes/box.h"
#include "shapes/cylinder.h"
#include "shapes/sphere.h"
#include <math.h>
#include <stdio.h>


void spatial_trigger_init(struct spatial_trigger* trigger, struct TransformSingleAxis* transform, struct spatial_trigger_type* type, uint16_t collision_layers) {
    trigger->transform = transform;
    trigger->type = type;
    spatial_trigger_recalc_bb(trigger);
    trigger->active_contacts = NULL;
    trigger->collision_layers = collision_layers;
    trigger->collision_group = 0;
}

void spatial_trigger_recalc_bb(struct spatial_trigger* trigger) {
    union spatial_trigger_data* data = &trigger->type->data; 
    spatial_trigger_type_recalc_bb(trigger->type, trigger->transform, &trigger->bounding_box);
}

void spatial_trigger_type_recalc_bb(struct spatial_trigger_type* type, struct TransformSingleAxis* transform, struct Box3D* result) {
    union spatial_trigger_data* data = &type->data; 
    switch (type->type) {
        case SPATIAL_TRIGGER_SPHERE:
            sphere_bounding_box(data, &transform->rotation, result);
            break;
        case SPATIAL_TRIGGER_CYLINDER:
            cylinder_bounding_box(data, &transform->rotation, result);
            break;
        case SPATIAL_TRIGGER_BOX:
            box_bounding_box(data, &transform->rotation, result);
            break;
        case SPATIAL_TRIGGER_WEDGE: {
            struct Box3D unrotated = {
                .min = {-data->wedge.angle.y * data->wedge.radius, -data->wedge.half_height, 0.0f},
                .max = {data->wedge.angle.y * data->wedge.radius, data->wedge.half_height, data->wedge.radius},
            };

            box3DRotate2D(&unrotated, &transform->rotation, result);
            break;
        }
    }

    struct Vector3 offset;
    vector3Add(&transform->position, &type->center, &offset);

    vector3Scale(&result->min, &result->min, transform->scale);
    vector3Add(&result->min, &offset, &result->min);
    vector3Add(&result->max, &offset, &result->max);
}

bool spatial_trigger_does_contain_point(struct spatial_trigger* trigger, struct Vector3* point) {
    if (!box3DContainsPoint(&trigger->bounding_box, point)) {
        return false;
    }

    struct Vector3 relative_pos;
    vector3Sub(point, &trigger->transform->position, &relative_pos);
    vector3Sub(&relative_pos, &trigger->type->center, &relative_pos);

    union spatial_trigger_data* data = &trigger->type->data; 

    switch (trigger->type->type)
    {
        case SPATIAL_TRIGGER_SPHERE: {
            float scaled_radius = data->sphere.radius * trigger->transform->scale;
            return vector3MagSqrd(&relative_pos) < scaled_radius * scaled_radius;
        }
        case SPATIAL_TRIGGER_CYLINDER: {
            float scaled_radius = data->cylinder.radius * trigger->transform->scale;
            return fabsf(relative_pos.y) < data->cylinder.half_height * trigger->transform->scale && 
                relative_pos.x * relative_pos.x + relative_pos.z * relative_pos.z < 
                scaled_radius * scaled_radius;
        }
        case SPATIAL_TRIGGER_BOX: {
            struct Vector3 unrotated;
            vector3RotateWith2Inv(&relative_pos, &trigger->transform->rotation, &unrotated);

            return fabsf(unrotated.x) < data->box.half_size.x * trigger->transform->scale &&
                fabsf(unrotated.y) < data->box.half_size.y * trigger->transform->scale &&
                fabsf(unrotated.z) < data->box.half_size.z * trigger->transform->scale;
        }
        case SPATIAL_TRIGGER_WEDGE: {
            if (fabsf(relative_pos.y) > data->wedge.half_height) {
                return false;
            }
            
            relative_pos.y = 0.0f;
            float scaled_radius = data->wedge.radius * trigger->transform->scale;
            if (vector3MagSqrd(&relative_pos) >= scaled_radius * scaled_radius) {
                return false;
            }

            struct Vector3 unrotated;
            vector3RotateWith2Inv(&relative_pos, &trigger->transform->rotation, &unrotated);

            struct Vector2* angle = &data->wedge.angle;

            if (angle->x >= 0.0f && unrotated.z <= 0.0f) {
                return false;
            }

            if (angle->x <= 0.0f && unrotated.z >= 0.0f) {
                return true;
            }

            float x_threshold = unrotated.z * angle->y;
            return fabsf(unrotated.x) * angle->x < x_threshold;
        }
        default:
            return false;
    }
}