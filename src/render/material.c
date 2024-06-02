#include "material.h"

void material_init(struct material* material) {
    material->block = 0;

    material->tex0.sprite = NULL;

    material->tex1.sprite = NULL;

    material->sort_priority = SORT_PRIORITY_OPAQUE;

    material->palette.tlut = 0;
    material->palette.idx = 0;
    material->palette.size = 0;
}

void material_free(struct material* material) {
    rspq_block_free(material->block);
    free(material->palette.tlut);
    material->palette.tlut = 0;
}