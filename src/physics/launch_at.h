#ifndef __PHYSICS_LAUNCH_AT_H__
#define __PHYSICS_LAUNCH_AT_H__

#include "../math/vector3.h"

void phys_launch_at(vector3_t* from, vector3_t* offset, float arc_height, vector3_t* out_vel);

#endif