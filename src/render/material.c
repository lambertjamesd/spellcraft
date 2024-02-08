#include "material.h"

void material_init(struct material* material) {
    material->list = glGenLists(1);

    material->tex0.sprite = NULL;
    material->tex0.gl_texture = 0;
}

void material_free(struct material* material) {
    glDeleteLists(material->list, 1);
}