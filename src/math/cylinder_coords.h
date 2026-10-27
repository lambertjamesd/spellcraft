#ifndef __MATH_CYLINDER_COORDS_H__
#define __MATH_CYLINDER_COORDS_H__

#include "vector2.h"
#include "vector3.h"

typedef struct {
    vector2_t direction;
    float radius;
    float height;
} cylinder_coords_t;

void cylinder_coords_from_vec3(vector3_t* offset, cylinder_coords_t* result);
void cylinder_coords_to_vec3(cylinder_coords_t* coords, vector3_t* offset);

#endif