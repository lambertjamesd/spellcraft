#include "mesh.h"

#include <malloc.h>

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "./coloru8.h"

void mesh_init(struct mesh* mesh, int submesh_count) {
    mesh->list = glGenLists(submesh_count);

    mesh->materials = malloc(sizeof(struct material) * submesh_count);

    for (int i = 0; i < submesh_count; ++i) {
        mesh->materials[i] = NULL;
    }
}


void mesh_destroy(struct mesh* mesh) {
    glDeleteLists(mesh->list, mesh->submesh_count);
    free(mesh->materials);
}

int mesh_attribute_size(int attributes) {
    int result = 0;

    if (attributes & MeshAttributesPosition) {
        result += sizeof(short) * 3;
    }

    if (attributes & MeshAttributesUV) {
        result += sizeof(short) * 2;
    }

    if (attributes & MeshAttributesColor) {
        result += sizeof(struct Coloru8);
    }

    if (attributes & MeshAttributesNormal) {
        result += sizeof(char) * 3;
    }

    return result;
}