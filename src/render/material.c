#include "material.h"

void material_init(struct material* material) {
    material->list = glGenLists(1);
}

void material_free(struct material* material) {
    glDeleteLists(material->list, 1);
}