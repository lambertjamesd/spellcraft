#ifndef __ENTITIES_JELLY_POT_H__
#define __ENTITIES_JELLY_POT_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

#define MAX_ACTIVE_JELLIES  3

struct jelly_pot {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    health_t health;
    entity_id spawned[MAX_ACTIVE_JELLIES];
    spatial_trigger_t vision;
    entity_id target;
    float attack_timer;
};

typedef struct jelly_pot jelly_pot_t;

void jelly_pot_init(jelly_pot_t* jelly_pot, struct jelly_pot_definition* definition, entity_id entity_id);
void jelly_pot_destroy(jelly_pot_t* jelly_pot, struct jelly_pot_definition* definition);
void jelly_pot_common_init();
void jelly_pot_common_destroy();

#endif