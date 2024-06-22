#include "push.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "assets.h"

void push_render(struct push* push, struct render_batch* batch) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (!target) {
        return;
    }
    
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->mesh.block = push->dash_trail_right.render_block;
    element->mesh.armature = NULL;
    element->mesh.transform = NULL;
    element->material = spell_assets_get()->dash_trail_material;

    dash_trail_update(&push->dash_trail_right, target->position);

    element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->mesh.block = push->dash_trail_left.render_block;
    element->mesh.armature = NULL;
    element->mesh.transform = NULL;
    element->material = spell_assets_get()->dash_trail_material;

    dash_trail_update(&push->dash_trail_left, target->position);
}

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options) {
    push->data_source = source;
    spell_data_source_retain(source);

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