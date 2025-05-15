#include "spatial_trigger.h"
#include "shapes/box.h"
#include "shapes/cylinder.h"
#include "shapes/sphere.h"
#include <math.h>


void spatial_trigger_init(struct spatial_trigger* trigger, struct TransformSingleAxis* transform, struct spatial_trigger_type* type) {
    trigger->transform = transform;
    trigger->type = type;
    spatial_trigger_recalc_bb(trigger);
    trigger->active_contacts = NULL;
}

void spatial_trigger_recalc_bb(struct spatial_trigger* trigger) {
    switch (trigger->type->type) {
        case SPATIAL_TRIGGER_SPHERE:
            sphere_bounding_box(&trigger->type->data, &trigger->transform->rotation, &trigger->bounding_box);
            break;
        case SPATIAL_TRIGGER_CYLINDER:
            cylinder_bounding_box(&trigger->type->data, &trigger->transform->rotation, &trigger->bounding_box);
            break;
        case SPATIAL_TRIGGER_BOX:
            box_bounding_box(&trigger->type->data, &trigger->transform->rotation, &trigger->bounding_box);
            break;
    }
    vector3Add(&trigger->bounding_box.min, &trigger->transform->position, &trigger->bounding_box.min);
    vector3Add(&trigger->bounding_box.max, &trigger->transform->position, &trigger->bounding_box.max);
}

bool spatial_trigger_does_contain_point(struct spatial_trigger* trigger, struct Vector3* point) {
    if (!box3DContainsPoint(&trigger->bounding_box, point)) {
        return false;
    }

    struct Vector3 relative_pos;
    vector3Sub(point, &trigger->transform->position, &relative_pos);

    switch (trigger->type->type)
    {
        case SPATIAL_TRIGGER_SPHERE:
            return vector3MagSqrd(&relative_pos) < trigger->type->data.sphere.radius * trigger->type->data.sphere.radius;
        case SPATIAL_TRIGGER_CYLINDER:
            return fabsf(relative_pos.y) < trigger->type->data.cylinder.half_height && 
                relative_pos.x * relative_pos.x + relative_pos.z * relative_pos.z < 
                trigger->type->data.cylinder.radius * trigger->type->data.cylinder.radius;
        case SPATIAL_TRIGGER_BOX: {
            struct Vector3 unrotated;
            vector3RotateWith2(&relative_pos, &trigger->transform->rotation, &unrotated);

            return fabsf(unrotated.x) < trigger->type->data.box.half_size.x &&
                fabsf(unrotated.y) < trigger->type->data.box.half_size.y &&
                fabsf(unrotated.z) < trigger->type->data.box.half_size.z;
        }
        default:
            return false;
    }
}