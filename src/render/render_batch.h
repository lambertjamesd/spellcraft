#ifndef __RENDER_RENDER_BATCH_H__
#define __RENDER_RENDER_BATCH_H__

#include "mesh.h"
#include "material.h"

#include "../math/matrix.h"

#define RENDER_BATCH_MAX_SIZE   256
#define RENDER_BATCH_TRANSFORM_COUNT    64

struct render_batch_element {
    struct material* material;
    GLuint list;
    mat4x4* transform;
};

struct render_batch {
    struct render_batch_element elements[RENDER_BATCH_MAX_SIZE];
    mat4x4 transform[RENDER_BATCH_TRANSFORM_COUNT];
    short element_count;
    short transform_count;
};

void render_batch_init(struct render_batch* batch);

struct render_batch_element* render_batch_add(struct render_batch* batch);

void render_batch_add_mesh(struct render_batch* batch, struct mesh* mesh, mat4x4* transform);

mat4x4* render_batch_get_transform(struct render_batch* batch);

void render_batch_finish(struct render_batch* batch);

#endif