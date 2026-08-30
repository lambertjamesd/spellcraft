#include "launch_at.h"

#include <math.h>
#include "../collision/dynamic_object.h"

float phys_fall_time(float height) {
    return sqrtf((-2.0f / GRAVITY_CONSTANT) * height);
}

void phys_launch_at(vector3_t* from, vector3_t* offset, float arc_height, vector3_t* out_vel) {
    float long_leg = arc_height + fabsf(offset->y);

    float long_leg_time = phys_fall_time(long_leg);
    float short_leg_time = phys_fall_time(arc_height);

    if (offset->y < 0.0f) {
        out_vel->y = -GRAVITY_CONSTANT * short_leg_time;
    } else {
        out_vel->y = -GRAVITY_CONSTANT * long_leg_time;
    }

    float total_time_inv = 1.0f / (long_leg_time + short_leg_time);
    out_vel->x = offset->x * total_time_inv;
    out_vel->z = offset->z * total_time_inv;
}