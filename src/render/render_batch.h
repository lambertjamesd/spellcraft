#ifndef __RENDER_RENDER_BATCH_H__
#define __RENDER_RENDER_BATCH_H__

#include "mesh.h"
#include "material.h"

#include "../math/matrix.h"

#define RENDER_BATCH_MAX_SIZE   256
#define RENDER_BATCH_TRANSFORM_COUNT    64
#define MAX_BILLBOARD_SPRITES           128

struct render_billboard_sprite {
    struct Vector3 position;
    float size;
};

enum render_batch_type {
    RENDER_BATCH_MESH,
    RENDER_BATCH_BILLBOARD,
};

struct render_batch_billboard_element {
    struct render_billboard_sprite* sprites;
    uint16_t sprite_count;
};

struct render_batch_element {
    struct material* material;
    uint16_t type;
    union {
        struct {
            GLuint list;
            mat4x4* transform;
        } mesh;
        struct render_batch_billboard_element billboard;
    };
};

struct render_batch {
    struct render_batch_element elements[RENDER_BATCH_MAX_SIZE];
    mat4x4 transform[RENDER_BATCH_TRANSFORM_COUNT];
    struct render_billboard_sprite sprites[MAX_BILLBOARD_SPRITES];
    short element_count;
    short transform_count;
    short sprite_count;
};

void render_batch_init(struct render_batch* batch);

struct render_batch_element* render_batch_add(struct render_batch* batch);

void render_batch_add_mesh(struct render_batch* batch, struct mesh* mesh, mat4x4* transform);

struct render_batch_billboard_element render_batch_get_sprites(struct render_batch* batch, int count);
mat4x4* render_batch_get_transform(struct render_batch* batch);

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj);

#endif