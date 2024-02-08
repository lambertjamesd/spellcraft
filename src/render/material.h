#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include <GL/gl.h>
#include <libdragon.h>

struct material_tex {
    sprite_t* sprite;
    GLuint gl_texture;
};

struct material {
    GLuint list;
    struct material_tex tex0;
};

void material_init(struct material* material);
void material_free(struct material* material);

#endif