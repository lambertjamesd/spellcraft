#ifndef __RENDER_RENDERABLE_H__
#define __RENDER_RENDERABLE_H__

#include "../math/transform.h"
#include "mesh.h"

struct renderable {
    struct Transform* transform;
    struct mesh* mesh;
};

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename);
void renderable_destroy(struct renderable* renderable);

#endif