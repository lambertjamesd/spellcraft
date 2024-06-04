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
    rdpq_texparms_t params;
    // rdpq_set_lookup_address
    // surface_make_placeholder_linear
    // sprite_t** frames;
    // uint16_t num_frames;
};

struct material_palette {
    uint16_t *tlut;
    uint16_t idx;
    uint16_t size;
};

struct material {
    rspq_block_t* block;
    struct material_tex tex0;
    struct material_tex tex1;
    struct material_palette palette;
    int16_t sort_priority;
};

void material_init(struct material* material);
void material_free(struct material* material);

// used to directly load a material
// materials loaded this way must be 
// released with material_release
void material_load(struct material* into, FILE* material_file);
void material_release(struct material* material);

#endif