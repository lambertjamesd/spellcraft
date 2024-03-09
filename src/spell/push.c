#include "push.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options) {
    push->data_source = source;
    spell_data_source_retain(source);
}

void push_destroy(struct push* push) {
    spell_data_source_release(push->data_source);
}

void push_update(struct push* push, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);
    
    if (target) {
        if (push->data_source->flags.cast_state == SPELL_CAST_STATE_INSTANT) {
            vector3AddScaled(&target->velocity, &push->data_source->direction, 15.0f, &target->velocity);
        } else {
            struct Vector3 targetVelocity;
            vector3Scale(&push->data_source->direction, &targetVelocity, 10.0f);
            vector3MoveTowards(&target->velocity, &targetVelocity, fixed_time_step * 60.0f, &target->velocity);
        }
    }

    if (!target || push->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
    }
}