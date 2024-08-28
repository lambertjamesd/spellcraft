#ifndef __MATH_TRANSFORM_MIXED_H__
#define __MATH_TRANSFORM_MIXED_H__

#include "vector3.h"
#include "transform.h"
#include "transform_single_axis.h"

enum transform_type {
    TRANSFORM_TYPE_BASIC,
    TRANSFORM_TYPE_SINGLE_AXIS,
};

struct transform_mixed {
    void* transform;
    enum transform_type type;
};

void transform_mixed_init(struct transform_mixed* transform_mixed, struct Transform* transform);
void transform_mixed_init_sa(struct transform_mixed* transform_mixed, struct TransformSingleAxis* transform);

struct Vector3* transform_mixed_get_position(struct transform_mixed* transform);

#endif