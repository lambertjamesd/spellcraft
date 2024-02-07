#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include <GL/gl.h>
#include <libdragon.h>

struct material {
    GLuint list;
    sprite_t* tex0_sprite;
};

void material_init(struct material* material);
void material_free(struct material* material);

#endif