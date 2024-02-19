#include "material.h"

void material_init(struct material* material) {
    material->list = glGenLists(1);

    material->tex0.sprite = NULL;
    material->tex0.gl_texture = 0;

    material->tex1.sprite = NULL;
    material->tex1.gl_texture = 0;

    material->sortPriority = SORT_PRIORITY_OPAQUE;
}

void material_free(struct material* material) {
    glDeleteLists(material->list, 1);
}