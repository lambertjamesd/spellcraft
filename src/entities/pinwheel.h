#ifndef __ENTITIES_PINWHEEL_H__
#define __ENTITIES_PINWHEEL_H__

#include "entity_deps.h"
#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../math/transform.h"

struct pinwheel {
    transform_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    vector3_t forward;
};

typedef struct pinwheel pinwheel_t;

void pinwheel_init(pinwheel_t* pinwheel, struct pinwheel_definition* definition, entity_id entity_id);
void pinwheel_destroy(pinwheel_t* pinwheel, struct pinwheel_definition* definition);
void pinwheel_common_init();
void pinwheel_common_destroy();

#endif