#ifndef __RENDER_RENDERABLE_H__
#define __RENDER_RENDERABLE_H__

#include "../math/transform.h"
#include "../math/transform_single_axis.h"
#include "armature.h"

enum renderable_type {
    RENDERABLE_TYPE_BASIC,
    RENDERABLE_TYPE_SINGLE_AXIS,
};

struct renderable {
    void* transform;
    struct tmesh* mesh;
    struct armature armature;
    struct material* force_material;
    struct tmesh** attachments;
    enum renderable_type type;
};

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename);
void renderable_single_axis_init(struct renderable* renderable, struct TransformSingleAxis* transform, const char* mesh_filename);

void renderable_destroy(struct renderable* renderable);


#endif