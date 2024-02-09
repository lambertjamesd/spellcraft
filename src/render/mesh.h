#ifndef __RENDER_MESH_H__
#define __RENDER_MESH_H__

#include <GL/gl.h>
#include "material.h"

enum MeshAttributes {
    MeshAttributesPosition = (1 << 0),
    MeshAttributesUV = (1 << 1),
    MeshAttributesColor = (1 << 2),
    MeshAttributesNormal = (1 << 3),

    MeshAttributesAll = MeshAttributesPosition | MeshAttributesUV | MeshAttributesColor | MeshAttributesNormal,
};

struct mesh {
    GLuint list;
    uint16_t submesh_count;
    struct material** materials;
};

void mesh_init(struct mesh* mesh, int submesh_count);
void mesh_destroy(struct mesh* mesh);

int meshAttributeSize(int attributes);

#endif