#ifndef __ENTITIES_WATER_WAVES_H__
#define __ENTITIES_WATER_WAVES_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../water/water.h"
#include "entity_deps.h"

struct water_waves {
    transform_sa_t transform;
    water_simulation_t simulation;
    renderable_t renderable;
};

typedef struct water_waves water_waves_t;

void water_waves_init(water_waves_t* water_waves, struct water_waves_definition* definition, entity_id entity_id);
void water_waves_destroy(water_waves_t* water_waves, struct water_waves_definition* definition);
void water_waves_common_init();
void water_waves_common_destroy();

#endif