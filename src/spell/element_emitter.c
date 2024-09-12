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

#define CYCLE_TIME  0.08f

#define FIRE_LENGTH         4.0f

#define MAX_RADIUS          0.5f
#define MAX_RANDOM_OFFSET   0.3f

#define START_FADE          0.75f

#define TIP_RISE            0.5f

void* fire_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->fire_sweep_mesh, pos, direction, radius);
}

void* fire_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->fire_around_mesh, pos, direction, radius);
}

static struct lightning_effect_def lightning_def = {
    .spread = 1.1f,
};

void* lightning_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return lightning_effect_new(pos, &lightning_def);
}

static struct lightning_effect_def lightning_around_def = {
    .spread = 3.14f,
};

void* lightning_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return lightning_effect_new(pos, &lightning_around_def);
}

void effect_nop_stop(void*) {};
bool effect_always_stopped(void*) {
    return false;
}

struct element_emitter_definition fire_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = sweep_minkowski_sum,
        .bounding_box = sweep_bounding_box,
        .data = {
            .sweep = {
                .range = {0.707f, 0.707f},
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

struct element_emitter_definition fire_around_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = cylinder_minkowski_sum,
        .bounding_box = cylinder_bounding_box,
        .data = {
            .cylinder = {
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_around_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

struct element_emitter_definition ice_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = sweep_minkowski_sum,
        .bounding_box = sweep_bounding_box,
        .data = {
            .sweep = {
                .range = {0.707f, 0.707f},
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

struct element_emitter_definition lightning_definition = {
    .element_type = ELEMENT_TYPE_LIGHTNING,
    .collider_type = {
        .minkowsi_sum = sweep_minkowski_sum,
        .bounding_box = sweep_bounding_box,
        .data = {
            .sweep = {
                .range = {0.707f, 0.707f},
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = lightning_effect_start,
    .on_effect_update = (on_effect_update)lightning_effect_set_position,
    .on_effect_stop = effect_nop_stop,
    .is_effect_running = effect_always_stopped,
    .effect_free = (effect_free)lightning_effect_free,
};

struct element_emitter_definition lightning_around_definition = {
    .element_type = ELEMENT_TYPE_LIGHTNING,
    .collider_type = {
        .minkowsi_sum = cylinder_minkowski_sum,
        .bounding_box = cylinder_bounding_box,
        .data = {
            .cylinder = {
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = lightning_around_effect_start,
    .on_effect_update = (on_effect_update)lightning_effect_set_position,
    .on_effect_stop = effect_nop_stop,
    .is_effect_running = effect_always_stopped,
    .effect_free = (effect_free)lightning_effect_free,
};

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
    element_emitter->dynamic_object.scale = FIRE_LENGTH;
    element_emitter->dynamic_object.is_trigger = 1;
    element_emitter->effect_definition = effect_definition;
    collision_scene_add(&element_emitter->dynamic_object);
    element_emitter->is_active = true;
}

void element_emitter_destroy(struct element_emitter* element_emitter) {
    spell_data_source_release(element_emitter->data_source);
    collision_scene_remove(&element_emitter->dynamic_object);

    if (element_emitter->effect) {
        element_emitter->effect_definition->effect_free(element_emitter->effect);
        element_emitter->effect = NULL;
    }
}

void element_emitter_update(struct element_emitter* element_emitter, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (element_emitter->is_active && element_emitter->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        element_emitter->effect_definition->on_effect_stop(
            element_emitter->effect
        );
        element_emitter->is_active = false;
    }

    element_emitter->effect_definition->on_effect_update(
        element_emitter->effect, 
        &element_emitter->transform.position, 
        &element_emitter->data_source->direction,
        element_emitter->effect_definition->scale
    );

    spell_data_source_apply_transform_sa(element_emitter->data_source, &element_emitter->transform);

    if (element_emitter->is_active) {
        health_apply_contact_damage(
            &element_emitter->dynamic_object, 
            element_emitter->effect_definition->damage_per_frame, 
            health_determine_damage_type(element_emitter->effect_definition->element_type)
        );
    } 
    
    if (!element_emitter->is_active && !element_emitter->effect_definition->is_effect_running(element_emitter->effect)) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }
}