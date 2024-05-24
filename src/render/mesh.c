#include "mesh.h"

#include <malloc.h>

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "coloru8.h"

void mesh_init(struct mesh* mesh, int submesh_count) {
    mesh->list = 0;//glGenLists(submesh_count);
    mesh->submesh_count = submesh_count;

    mesh->materials = malloc(sizeof(struct material) * submesh_count);
    mesh->material_flags = malloc(sizeof(*mesh->material_flags) * submesh_count);

    for (int i = 0; i < submesh_count; ++i) {
        mesh->materials[i] = NULL;
        mesh->material_flags[i] = 0;
    }

    armature_definition_init(&mesh->armature, 0);
}

void mesh_destroy(struct mesh* mesh) {
    glDeleteLists(mesh->list, mesh->submesh_count);
    free(mesh->materials);
    free(mesh->material_flags);

    armature_definition_destroy(&mesh->armature);
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

    if (attributes & MeshAttributesMatrixIndex) {
        result += sizeof(char);
    }

    return result;
}