#ifndef __RENDER_DEFS_H__
#define __RENDER_DEFS_H__

#include "../math/vector3.h"

#define MODEL_SCALE 64
#define WORLD_SCALE 32

#define MODEL_WORLD_SCALE   ((float)WORLD_SCALE / (float)MODEL_SCALE)

void pack_position_vector(struct Vector3* input, short output[3]);

#endif