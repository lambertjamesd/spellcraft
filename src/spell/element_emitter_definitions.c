#include "element_emitter_definitions.h"

#include "assets.h"
#include "../effects/lightning_effect.h"
#include "../effects/scale_in_fade_out.h"
#include "../collision/shapes/sweep.h"
#include "../collision/shapes/cylinder.h"

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