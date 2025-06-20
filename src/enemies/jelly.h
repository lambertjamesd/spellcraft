#ifndef __ENEMIES_JELLY_H__
#define __ENEMIES_JELLY_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../scene/scene_definition.h"

struct jelly {
    struct TransformSingleAxis transform;
    struct renderable renderable;
};

void jelly_init(struct jelly* jelly, struct jelly_definition* definition);
void jelly_destroy(struct jelly* jelly);

#endif