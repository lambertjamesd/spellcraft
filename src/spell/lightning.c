#include "lightning.h"

#include "../collision/shapes/sweep.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"

static struct dynamic_object_type lightning_object_type = {
    .minkowsi_sum = dynamic_object_sweep_minkowski_sum,
    .bounding_box = dynamic_object_sweep_bounding_box,
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

void lightning_apply_transform(struct lightning* lightning) {
    lightning->position = lightning->data_source->position;

    lightning->rotation.x = lightning->data_source->direction.z;
    lightning->rotation.y = -lightning->data_source->direction.x;

    vector2Normalize(&lightning->rotation, &lightning->rotation);
}

void lightning_init(struct lightning* lightning, struct spell_data_source* source) {
    lightning->effect = lightning_effect_new(&source->position, &lightning_def);
    lightning->data_source = source;
    spell_data_source_retain(source);

    lightning_apply_transform(lightning);

    dynamic_object_init(
        entity_id_new(), 
        &lightning->dynamic_object, 
        &lightning_object_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &lightning->position, 
        &lightning->rotation
    );

    collision_scene_add(&lightning->dynamic_object);
}

void lightning_destroy(struct lightning* lightning) {
    spell_data_source_release(lightning->data_source);
    lightning_effect_free(lightning->effect);
    collision_scene_remove(&lightning->dynamic_object);
}

void lightning_apply_damage(struct dynamic_object* dyanmic_object) {
    struct contact* curr = dyanmic_object->active_contacts;

    while (curr) {
        struct health* target_health = health_get(curr->other_object);
        curr = curr->next;

        if (!target_health) {
            continue;
        }

        health_damage(target_health, 1.0f, dyanmic_object->entity_id, DAMAGE_TYPE_FIRE);
    }
}

void lightning_update(struct lightning* lightning, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    lightning_effect_set_position(lightning->effect, &lightning->data_source->position, &lightning->data_source->direction);
    lightning_apply_transform(lightning);

    if (lightning->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }

    lightning_apply_damage(&lightning->dynamic_object);
}