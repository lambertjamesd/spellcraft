#ifndef __COLLISION_SHAPE_SWEEP_H__
#define __COLLISION_SHAPE_SWEEP_H__

#include "../../math/vector2.h"
#include "../../math/vector3.h"
#include "../../math/box3d.h"

void dynamic_object_sweep_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_sweep_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

#endif