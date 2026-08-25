#ifndef __ENTITIES_REPAIR_INTERACTION_H__
#define __ENTITIES_REPAIR_INTERACTION_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

struct repair_interaction {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    interactable_t interactable;
    boolean_variable is_repaired;
    scene_entry_point on_interact;
};

typedef struct repair_interaction repair_interaction_t;

void repair_interaction_init(repair_interaction_t* repair_interaction, struct repair_interaction_definition* definition, entity_id entity_id);
void repair_interaction_destroy(repair_interaction_t* repair_interaction, struct repair_interaction_definition* definition);
void repair_interaction_common_init();
void repair_interaction_common_destroy();

#endif