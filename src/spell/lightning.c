#include "lightning.h"

#include "../collision/shapes/sweep.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"

static struct dynamic_object_type lightning_object_type = {
    .minkowsi_sum = sweep_minkowski_sum,
    .bounding_box = sweep_bounding_box,
    .data = {
        .sweep = {
            .range = {0.707f, 0.707f},
            .radius = 2.0f,
            .half_height = 0.25f,
        }
    }
};

static struct lightning_effect_def lightning_def = {
    .spread = 1.1f,
};

void lightning_init(struct lightning* lightning, struct spell_data_source* source) {
    lightning->effect = lightning_effect_new(&source->position, &lightning_def);
    lightning->data_source = source;
    spell_data_source_retain(source);

    spell_data_source_apply_transform_sa(lightning->data_source, &lightning->transform);

    dynamic_object_init(
        entity_id_new(), 
        &lightning->dynamic_object, 
        &lightning_object_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &lightning->transform.position, 
        &lightning->transform.rotation
    );

    lightning->dynamic_object.is_trigger = 1;

    collision_scene_add(&lightning->dynamic_object);
}

void lightning_destroy(struct lightning* lightning) {
    spell_data_source_release(lightning->data_source);
    lightning_effect_free(lightning->effect);
    collision_scene_remove(&lightning->dynamic_object);
}

void lightning_update(struct lightning* lightning, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (lightning->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }

    lightning_effect_set_position(lightning->effect, &lightning->data_source->position, &lightning->data_source->direction);
    
    spell_data_source_apply_transform_sa(lightning->data_source, &lightning->transform);

    health_apply_contact_damage(&lightning->dynamic_object, 1.0f, DAMAGE_TYPE_LIGHTING);
}