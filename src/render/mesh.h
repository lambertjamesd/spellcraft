#ifndef __RENDER_MESH_H__
#define __RENDER_MESH_H__

#include <GL/gl.h>

enum MeshAttributes {
    MeshAttributesPosition = (1 << 0),
    MeshAttributesUV = (1 << 1),
    MeshAttributesColor = (1 << 2),
    MeshAttributesNormal = (1 << 3),

    MeshAttributesAll = MeshAttributesPosition | MeshAttributesUV | MeshAttributesColor | MeshAttributesNormal,
};

struct Mesh {
    GLuint list;
    uint16_t submeshCount;
};

void meshInit(struct Mesh* mesh, int submeshCount);

int meshAttributeSize(int attributes);

#endif