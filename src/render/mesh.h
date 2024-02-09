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

enum mesh_material_flags {
    // the memory from this material is managed by mesh loader
    MESH_MATERIAL_FLAGS_EMBEDDED = (1 << 0),
};

struct mesh {
    GLuint list;
    uint16_t submesh_count;
    struct material** materials;
    uint8_t* material_flags;
};

void mesh_init(struct mesh* mesh, int submesh_count);
void mesh_destroy(struct mesh* mesh);

int mesh_attribute_size(int attributes);

#endif