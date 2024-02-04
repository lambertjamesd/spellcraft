#include "mesh.h"

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "./coloru8.h"

void meshInit(struct Mesh* mesh, int submeshCount) {
    mesh->list = glGenLists(submeshCount);
}

int meshAttributeSize(int attributes) {
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