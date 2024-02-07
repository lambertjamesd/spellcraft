#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include <GL/gl.h>

struct material {
    GLuint list;
};

void material_init(struct material* material);
void material_free(struct material* material);

#endif