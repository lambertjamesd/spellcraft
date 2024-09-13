#include "move_towards.h"

#include "../time/time.h"
#include "../math/mathf.h"
#include <math.h>

// dP = v*t + 0.5*a*t*t
// t = v/a
// stop_distance = 0.5*v*v/a

// sqrtf(stop_distance * a * 2)

void move_towards(
    struct Vector3* position,
    float* speed,
    struct Vector3* target,
    struct move_towards_parameters* parameters
) {
    float distance = sqrtf(vector3DistSqrd(position, target));

    float target_speed = parameters->max_speed;

    float max_speed_sqr = distance * parameters->max_accel;

    if (max_speed_sqr < parameters->max_speed * parameters->max_speed) {
        target_speed = sqrtf(target_speed);
    }

    *speed = mathfMoveTowards(*speed, target_speed, parameters->max_accel * fixed_time_step);

    vector3MoveTowards(position, target, *speed * fixed_time_step, position);
}