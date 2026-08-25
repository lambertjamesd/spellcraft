#include "item_pickup.h"    

#include "../collision/shapes/cylinder.h"

static dynamic_object_type_t item_pickup_collider = {
    CYLINDER_COLLIDER(0.4f, 0.4f),
    .center = {0.0f, 0.4f, 0.0f},
};

void item_pickup_interact(struct interactable* interactable, entity_id from) {
    item_pickup_t* item_pickup = (item_pickup_t*)interactable->data;

    entity_despawn(interactable->id);
    expression_set_bool(item_pickup->has_item, true);
    expression_set_integer(item_pickup->has_item_count, expression_get_integer(item_pickup->has_item_count) + 1);
}
    
void item_pickup_init(item_pickup_t* item_pickup, struct item_pickup_definition* definition, entity_id entity_id) {
    transformSaInit(&item_pickup->transform, &definition->position, &definition->rotation, 1.0f);

    if (expression_get_bool(definition->has_item)) {
        item_pickup->is_active = false;
        return;
    }

    renderable_single_axis_init(&item_pickup->renderable, &item_pickup->transform, definition->mesh);
    render_scene_add_renderable(&item_pickup->renderable, 2.0f);

    dynamic_object_init(entity_id, &item_pickup->collider, &item_pickup_collider, COLLISION_LAYER_TANGIBLE, &item_pickup->transform.position, NULL);

    collision_scene_add(&item_pickup->collider);

    item_pickup->collider.weight_class = WEIGHT_CLASS_GHOST;
    item_pickup->collider.is_fixed = true;

    interactable_init(&item_pickup->interactable, entity_id, INTERACT_TYPE_TAKE, item_pickup_interact, item_pickup);

    item_pickup->has_item = definition->has_item;
    item_pickup->has_item_count = definition->has_item_count;

    item_pickup->is_active = true;
}

void item_pickup_destroy(item_pickup_t* item_pickup, struct item_pickup_definition* definition) {
    if (!item_pickup->is_active) {
        return;
    }

    render_scene_remove(&item_pickup->renderable);
    renderable_destroy(&item_pickup->renderable);

    collision_scene_remove(&item_pickup->collider);
    interactable_destroy(&item_pickup->interactable);
}

void item_pickup_common_init() {

}

void item_pickup_common_destroy() {

}
