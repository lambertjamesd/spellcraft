#ifndef __OVERWORLD_RENDER_H__
#define __OVERWORLD_RENDER_H__

#include "overworld.h"

#include "../math/matrix.h"
#include "../math/vector3.h"
#include "../render/frame_alloc.h"
#include "../render/tmesh.h"

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct frame_memory_pool* pool);

#endif