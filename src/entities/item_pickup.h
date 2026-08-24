#ifndef __ENTITIES_ITEM_PICKUP_H__
#define __ENTITIES_ITEM_PICKUP_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

struct item_pickup {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    interactable_t interactable;
    boolean_variable has_item;
    integer_variable has_item_count;
    bool is_active;
};

typedef struct item_pickup item_pickup_t;

void item_pickup_init(item_pickup_t* item_pickup, struct item_pickup_definition* definition, entity_id entity_id);
void item_pickup_destroy(item_pickup_t* item_pickup, struct item_pickup_definition* definition);
void item_pickup_common_init();
void item_pickup_common_destroy();

#endif