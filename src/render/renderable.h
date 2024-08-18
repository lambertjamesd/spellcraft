#ifndef __RENDER_RENDERABLE_H__
#define __RENDER_RENDERABLE_H__

#include "../math/transform.h"
#include "../math/transform_single_axis.h"
#include "armature.h"

struct renderable {
    struct Transform* transform;
    struct tmesh* mesh;
    struct armature armature;
    struct material* force_material;
    struct tmesh** attachments;
};

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename);
void renderable_destroy(struct renderable* renderable);

struct renderable_single_axis {
    struct TransformSingleAxis* transform;
    struct tmesh* mesh;
    struct armature armature;
    struct material* force_material;
    struct tmesh** attachments;
};

void renderable_single_axis_init(struct renderable_single_axis* renderable, struct TransformSingleAxis* transform, const char* mesh_filename);
void renderable_single_axis_destroy(struct renderable_single_axis* renderable);

#endif