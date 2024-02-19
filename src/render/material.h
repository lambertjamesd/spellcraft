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
};

struct material {
    GLuint list;
    struct material_tex tex0;
    struct material_tex tex1;
    int16_t sortPriority;
};

void material_init(struct material* material);
void material_free(struct material* material);

#endif