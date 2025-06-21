#ifndef __ENEMIES_JELLY_H__
#define __ENEMIES_JELLY_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../scene/scene_definition.h"
#include "../entity/health.h"
#include "../collision/dynamic_object.h"
#include "../render/tmesh.h"

struct jelly {
    struct TransformSingleAxis transform;
    struct tmesh* mesh;
    struct health health;
    struct dynamic_object collider;

    uint16_t needs_new_radius: 1;
};

void jelly_init(struct jelly* jelly, struct jelly_definition* definition);
void jelly_destroy(struct jelly* jelly);

#endif