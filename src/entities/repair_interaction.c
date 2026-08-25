#include "repair_interaction.h"    
    
#include "../collision/shapes/cylinder.h"
#include "../scene/scene.h"

static dynamic_object_type_t repair_interaction_collider = {
    CYLINDER_COLLIDER(0.4f, 0.4f),
    .center = {0.0f, 0.4f, 0.0f},
};

void repair_interaction_interact(struct interactable* interactable, entity_id from) {
    repair_interaction_t* repair_interaction = (repair_interaction_t*)interactable->data;
    scene_queue_next(repair_interaction->on_interact);
}

void repair_interaction_init(repair_interaction_t* repair_interaction, struct repair_interaction_definition* definition, entity_id entity_id) {
    transformSaInit(&repair_interaction->transform, &definition->position, &definition->rotation, 1.0f);

    bool is_repaired = expression_get_bool(definition->is_repaired);

    renderable_single_axis_init(
        &repair_interaction->renderable,
        &repair_interaction->transform,
        is_repaired ? definition->repaired_mesh : definition->broken_mesh
    );
    render_scene_add_renderable(&repair_interaction->renderable, 2.0f);

    dynamic_object_init(
        entity_id, 
        &repair_interaction->collider, 
        &repair_interaction_collider, 
        COLLISION_LAYER_TANGIBLE, 
        &repair_interaction->transform.position, 
        NULL
    );

    repair_interaction->collider.weight_class = WEIGHT_CLASS_GHOST;
    repair_interaction->collider.is_fixed = true;

    collision_scene_add(&repair_interaction->collider);

    interactable_init(
        &repair_interaction->interactable,
        entity_id,
        is_repaired ? INTERACT_TYPE_NONE : INTERACT_TYPE_CHECK,
        repair_interaction_interact,
        repair_interaction
    );

    repair_interaction->is_repaired = definition->is_repaired;
    repair_interaction->on_interact = definition->on_interact;
}

void repair_interaction_destroy(repair_interaction_t* repair_interaction, struct repair_interaction_definition* definition) {
    render_scene_remove(&repair_interaction->renderable);
    renderable_destroy(&repair_interaction->renderable);
    collision_scene_remove(&repair_interaction->collider);
    interactable_destroy(&repair_interaction->interactable);
}

void repair_interaction_common_init() {

}

void repair_interaction_common_destroy() {

}
