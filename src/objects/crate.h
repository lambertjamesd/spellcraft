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
    entity_spawner line_path_spawner;
    entity_id line_path_id;
    uint8_t current_edge;
};

void crate_init(struct crate* crate, struct crate_definition* definition, entity_id id);
void crate_destroy(struct crate* crate);
void crate_common_init();
void crate_common_destroy();

#endif