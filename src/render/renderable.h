#ifndef __RENDER_RENDERABLE_H__
#define __RENDER_RENDERABLE_H__

#include "../math/transform.h"
#include "./mesh.h"

struct renderable {
    struct Transform transform;
    struct mesh* mesh;
};

#endif