#ifndef __OBJECTS_CRATE_H__
#define __OBJECTS_CRATE_H__

#include "../scene/world_definition.h"

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"

struct crate {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;
    struct dynamic_object dynamic_object;
    int render_id;
};

void crate_init(struct crate* crate, struct crate_definition* definition);
void crate_destroy(struct crate* crate);

#endif