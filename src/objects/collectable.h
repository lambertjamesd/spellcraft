#ifndef __OBJECTS_COLLECTABLE_H__
#define __OBJECTS_COLLECTABLE_H__

#include "../math/vector3.h"
#include "../collision/dynamic_object.h"
#include "../scene/world_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"

struct collectable {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;
    enum collectable_type type;
    struct dynamic_object dynamic_object;
};

void collectable_assets_load();

void collectable_init(struct collectable* collectable, struct collectable_definition* definition);
void collectable_collected(struct collectable* collectable);
void collectable_destroy(struct collectable* collectable);

struct collectable* collectable_get(entity_id id);

#endif