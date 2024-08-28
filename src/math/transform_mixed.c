#include "transform_mixed.h"

void transform_mixed_init(struct transform_mixed* transform_mixed, struct Transform* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_BASIC;
}

void transform_mixed_init_sa(struct transform_mixed* transform_mixed, struct TransformSingleAxis* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

struct Vector3* transform_mixed_get_position(struct transform_mixed* transform) {
    return transform->transform;
}