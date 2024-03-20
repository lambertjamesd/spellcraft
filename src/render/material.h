#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include <GL/gl.h>
#include <libdragon.h>

#define SORT_PRIORITY_NO_DEPTH_TEST -100
#define SORT_PRIORITY_OPAQUE        0
#define SORT_PRIORITY_DECAL         100
#define SORT_PRIORITY_TRANSPARENT   200

struct material_tex {
    sprite_t* sprite;
    GLuint gl_texture;
    rdpq_texparms_t params;
    // rdpq_set_lookup_address
    // surface_make_placeholder_linear
    // sprite_t** frames;
    // uint16_t num_frames;
};

struct material {
    rspq_block_t* block;
    struct material_tex tex0;
    struct material_tex tex1;
    int16_t sort_priority;
};

void material_init(struct material* material);
void material_free(struct material* material);

#endif