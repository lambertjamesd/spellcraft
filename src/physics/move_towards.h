#ifndef __PHYSICS_MOVE_TOWARDS_H__
#define __PHYSICS_MOVE_TOWARDS_H__

#include "../math/vector3.h"

struct move_towards_parameters {
    float max_speed;
    float max_accel_tangent;
    float max_accel_dir;
    float damping;
};

void move_towards(
    struct Vector3* position,
    struct Vector3* velocity,
    struct Vector3* target,
    struct move_towards_parameters* parameters
);

#endif