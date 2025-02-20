#include "jump.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "../entity/health.h"
#include "../util/flags.h"
#include "assets.h"

void jump_init(struct jump* jump, struct spell_data_source* source, struct spell_event_options event_options, struct jump_definition* definition) {
    jump->data_source = spell_data_source_retain(source);
    
    jump->did_start = false;
    jump->max_float_time = definition->max_hover_time;
    jump->definition = definition;

    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (target) {
        target->is_jumping += 1;
    }
}

void jump_destroy(struct jump* jump) {
    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (target) {
        target->is_jumping -= 1;
    }
}

bool jump_update(struct jump* jump, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct dynamic_object* target = collision_scene_find_object(jump->data_source->target);

    if (!target) {
        return false;
    }

    float requested_amount = jump->definition->mana_per_second * fixed_time_step;
    float power_ratio = mana_pool_request(&spell_sources->mana_pool, requested_amount) / requested_amount;

    jump->max_float_time -= fixed_time_step;

    if (jump->max_float_time < 0.0f || power_ratio == 0.0f) {
        return false;
    }

    if (!jump->did_start) {
        jump->did_start = true;

        if (dynamic_object_is_grounded(target)) {
            target->velocity.y += jump->definition->initial_impulse * power_ratio;
        }
    } else {
        target->velocity.y += jump->definition->hover_accel * fixed_time_step * power_ratio;
    }

    return jump->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}

struct jump_definition jump_def_fire = {
    .initial_impulse = 3.0f,
    .hover_accel = 10.0f,
    .max_hover_time = 0.5f,
    .mana_per_second = 3.0f,
};

struct jump_definition jump_def_water = {
    .initial_impulse = 0.5f,
    .hover_accel = 10.0f,
    .max_hover_time = 5.0f,
    .mana_per_second = 1.0f,
};