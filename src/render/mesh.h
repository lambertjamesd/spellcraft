#ifndef __RENDER_MESH_H__
#define __RENDER_MESH_H__

#include <GL/gl.h>

struct Mesh {
    GLuint list;
};

void meshInit(struct Mesh* mesh);

#endif