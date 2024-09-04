#include "move_towards.h"

#include "../time/time.h"
#include <math.h>

// dP = v*t + 0.5*a*t*t
// t = v/a
// stop_distance = 0.5*v*v/a

void move_towards(
    struct Vector3* position,
    struct Vector3* velocity,
    struct Vector3* target,
    struct move_towards_parameters* parameters
) {
    struct Vector3 offset;
    struct Vector3 direction;

    vector3Sub(target, position, &offset);

    vector3Normalize(&offset, &direction);
    
    float distance = vector3Dot(&offset, &direction);

    if (vector3MagSqrd(&direction) < 0.00001f) {
        direction = gRight;
    }

    float dir_velocity = vector3Dot(&direction, velocity);

    struct Vector3 tangent_velocity;
    vector3AddScaled(velocity, &direction, -dir_velocity, &tangent_velocity);
    vector3MoveTowards(&tangent_velocity, &gZeroVec, parameters->max_accel_tangent * fixed_time_step, &tangent_velocity);

    if (dir_velocity < 0.0f) {
        dir_velocity += fixed_time_step * parameters->max_accel_dir;
    } else {
        float stop_distance = 0.5f * dir_velocity * dir_velocity / parameters->max_accel_dir;

        if (stop_distance < distance) {
            dir_velocity += fixed_time_step * parameters->max_accel_dir;
        } else if (distance > 0.00001f) {
            float accel = 0.5f * dir_velocity * dir_velocity / distance;
            dir_velocity -= fixed_time_step * accel;
        }
    }

    vector3AddScaled(&tangent_velocity, &direction, dir_velocity, velocity);

    float speedSqrd = vector3MagSqrd(velocity);

    if (speedSqrd > parameters->max_speed * parameters->max_speed) {
        vector3Scale(velocity, velocity, parameters->max_speed / sqrtf(speedSqrd));
    }
    vector3AddScaled(position, velocity, fixed_time_step, position);

    vector3Sub(target, position, &offset);

    if (vector3Dot(&offset, &direction) <= 0.0f) {
        *velocity = gZeroVec;
        *position = *target;
    }
}