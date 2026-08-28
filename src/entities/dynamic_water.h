#ifndef __ENTITIES_DYNAMIC_WATER_H__
#define __ENTITIES_DYNAMIC_WATER_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

struct dynamic_water {
    transform_sa_t transform;
    renderable_t renderable;
    spatial_trigger_t trigger;
    spatial_trigger_type_t trigger_type;

    float start_level;
    float other_level;

    boolean_variable is_other_level;
};

typedef struct dynamic_water dynamic_water_t;

void dynamic_water_init(dynamic_water_t* dynamic_water, struct dynamic_water_definition* definition, entity_id entity_id);
void dynamic_water_destroy(dynamic_water_t* dynamic_water, struct dynamic_water_definition* definition);
void dynamic_water_common_init();
void dynamic_water_common_destroy();

#endif