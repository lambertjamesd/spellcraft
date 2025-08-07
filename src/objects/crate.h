#ifndef __OBJECTS_CRATE_H__
#define __OBJECTS_CRATE_H__

#include "../scene/scene_definition.h"

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../entity/health.h"

struct crate {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object dynamic_object;
    struct health health;
};

void crate_init(struct crate* crate, struct crate_definition* definition, entity_id id);
void crate_destroy(struct crate* crate);

#endif