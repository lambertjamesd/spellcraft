#include "push.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "assets.h"

int which_one = 0;

#define PUSH_STRENGTH       10.0f
#define MANA_PER_SECOND     10.0f

void push_render(struct push* push, struct render_batch* batch) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (!target) {
        return;
    }

    dash_trail_render(&push->dash_trail_right, target->position, batch);
    dash_trail_render(&push->dash_trail_left, target->position, batch);
}

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options) {
    push->data_source = source;
    spell_data_source_retain(source);
    mana_regulator_init(&push->mana_regulator, event_options.burst_mana);

    dash_trail_init(&push->dash_trail_right, &source->position, false);
    dash_trail_init(&push->dash_trail_left, &source->position, true);

    render_scene_add(&push->data_source->position, 4.0f, (render_scene_callback)push_render, push);
}

void push_destroy(struct push* push) {
    spell_data_source_release(push->data_source);
    render_scene_remove(push);
    dash_trail_destroy(&push->dash_trail_right);
    dash_trail_destroy(&push->dash_trail_left);
}

void push_update(struct push* push, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (target) {
        if (push->data_source->flags.cast_state == SPELL_CAST_STATE_INSTANT) {
            vector3AddScaled(&target->velocity, &push->data_source->direction, 15.0f, &target->velocity);
        } else {
            float power_ratio = mana_regulator_request(&push->mana_regulator, &spell_sources->mana_pool, MANA_PER_SECOND * scaled_time_step) * scaled_time_step_inv * (1.0f / MANA_PER_SECOND);

            if (power_ratio == 0.0f) {
                spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0, 0.0f);
                return;
            }

            struct Vector3 targetVelocity;
            vector3Scale(&push->data_source->direction, &targetVelocity, PUSH_STRENGTH * power_ratio);
            vector3MoveTowards(&target->velocity, &targetVelocity, scaled_time_step * 60.0f * power_ratio, &target->velocity);
        }
    }

    if (!target || push->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0, 0.0f);
    }
}