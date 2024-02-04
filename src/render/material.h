#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include <GL/gl.h>

struct Material {
    GLuint list;
};

void materialInit(struct Material* material);

#endif