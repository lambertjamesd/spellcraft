#include "cylinder_coords.h"

#include <math.h>

void cylinder_coords_from_vec3(vector3_t* offset, cylinder_coords_t* result) {
    result->height = offset->y;

    vector2_t horz_offset = {
        .x = offset->x,
        .y = offset->z,
    };
    
    result->radius = sqrtf(vector2MagSqr(&horz_offset));

    if (result->radius < 0.0001f) {
        result->direction = gRight2;
    } else {
        vector2Scale(&horz_offset, 1.0f / result->radius, &result->direction);
    }
}

void cylinder_coords_to_vec3(cylinder_coords_t* coords, vector3_t* offset) {
    offset->x = coords->direction.x * coords->radius;
    offset->y = coords->height;
    offset->z = coords->direction.y * coords->radius;
}