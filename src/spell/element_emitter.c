#include "element_emitter.h"

#include "assets.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/sweep.h"
#include "../collision/shapes/cylinder.h"
#include "../entity/health.h"
#include "../math/mathf.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../effects/lightning_effect.h"

void element_emitter_init(struct element_emitter* element_emitter, struct spell_data_source* source, struct spell_event_options event_options, struct element_emitter_definition* effect_definition) {
    element_emitter->data_source = source;
    spell_data_source_retain(source);

    spell_data_source_apply_transform_sa(element_emitter->data_source, &element_emitter->transform);

    element_emitter->effect = effect_definition->on_effect_start(&element_emitter->transform.position, &source->direction, effect_definition->scale);

    dynamic_object_init(
        entity_id_new(), 
        &element_emitter->dynamic_object, 
        &effect_definition->collider_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &element_emitter->transform.position, 
        &element_emitter->transform.rotation
    );
    element_emitter->dynamic_object.scale = effect_definition->scale;
    element_emitter->dynamic_object.is_trigger = 1;
    element_emitter->effect_definition = effect_definition;
    collision_scene_add(&element_emitter->dynamic_object);
    element_emitter->is_active = true;
}

void element_emitter_destroy(struct element_emitter* element_emitter) {
    spell_data_source_release(element_emitter->data_source);
    collision_scene_remove(&element_emitter->dynamic_object);

    if (element_emitter->effect) {
        // not paired with effect_malloc()
        element_emitter->effect_definition->effect_free(element_emitter->effect);
        element_emitter->effect = NULL;
    }
}

bool element_emitter_update(struct element_emitter* element_emitter, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (element_emitter->is_active && element_emitter->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        element_emitter->effect_definition->on_effect_stop(
            element_emitter->effect
        );
        element_emitter->is_active = false;
    }

    float mana_requested = fixed_time_step * element_emitter->effect_definition->mana_per_second;

    float mana_ratio = mana_pool_request(
        &spell_sources->mana_pool, 
        mana_requested
    ) / mana_requested;

    if (mana_ratio == 0.0f) {
        return false;
    }

    element_emitter->effect_definition->on_effect_update(
        element_emitter->effect, 
        &element_emitter->transform.position, 
        &element_emitter->data_source->direction,
        element_emitter->effect_definition->scale * mana_ratio
    );

    spell_data_source_apply_transform_sa(element_emitter->data_source, &element_emitter->transform);


    if (element_emitter->is_active) {
        struct damage_source source = {
            .amount = element_emitter->effect_definition->damage_per_frame * mana_ratio,
            .type = health_determine_damage_type(element_emitter->effect_definition->element_type),
        };

        health_apply_contact_damage(
            &element_emitter->dynamic_object, 
            &source, 
            NULL
        );
    } 

    return element_emitter->is_active || element_emitter->effect_definition->is_effect_running(element_emitter->effect);
}