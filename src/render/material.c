#include "material.h"

void materialInit(struct Material* material) {
    material->list = glGenLists(1);
}