#include "transform_mixed.h"

void transform_mixed_init(struct transform_mixed* transform_mixed, struct Transform* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_BASIC;
}

void transform_mixed_init_sa(struct transform_mixed* transform_mixed, struct TransformSingleAxis* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

void transform_mixed_init_pos(struct transform_mixed* transform_mixed, struct Vector3* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_POSITION;
}

struct Vector3* transform_mixed_get_position(struct transform_mixed* transform) {
    return transform->transform;
}

bool transform_rotate_towards(struct transform_mixed* transform, struct Vector3* forward, float max_radians) {
    struct Vector2 current_dir;
    struct Vector2 target_dir;

    vector2LookDir(&target_dir, forward);

    switch (transform->type) {
        case TRANSFORM_TYPE_BASIC: {
            struct Vector3 current_forward;
            quatMultVector(&((struct Transform*)transform->transform)->rotation, &gForward, &current_forward);
            vector2LookDir(&current_dir, &current_forward);
            break;
        }
        case TRANSFORM_TYPE_SINGLE_AXIS: {
            current_dir = ((struct TransformSingleAxis*)transform->transform)->rotation;
            break;
        }
        default:
            return true;
    }

    struct Vector2 max_rotate;
    vector2ComplexFromAngle(max_radians, &max_rotate);

    bool result = vector2RotateTowards(&current_dir, &target_dir, &max_rotate, &current_dir);

    switch (transform->type) {
        case TRANSFORM_TYPE_BASIC: {
            quatAxisComplex(&gUp, &current_dir, &((struct Transform*)transform->transform)->rotation);
            break;
        }
        case TRANSFORM_TYPE_SINGLE_AXIS: {
            ((struct TransformSingleAxis*)transform->transform)->rotation = current_dir;
            break;
        }
        default:
            break;
    }

    return result;
}