#include "push.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "../entity/health.h"
#include "../util/flags.h"
#include "assets.h"

int which_one = 0;

static uint8_t is_burst_dash[] = {
    [ELEMENT_TYPE_NONE] = false,
    [ELEMENT_TYPE_FIRE] = true,
    [ELEMENT_TYPE_ICE] = false,
    [ELEMENT_TYPE_LIGHTNING] = true,
};

static float push_strength[] = {
    [ELEMENT_TYPE_NONE] = 10.0f,
    [ELEMENT_TYPE_FIRE] = 10.0f,
    [ELEMENT_TYPE_ICE] = 10.0f,
    [ELEMENT_TYPE_LIGHTNING] = 10.0f,
};

static float mana_per_second[] = {
    [ELEMENT_TYPE_NONE] = 10.0f,
    [ELEMENT_TYPE_FIRE] = 10.0f,
    [ELEMENT_TYPE_ICE] = 10.0f,
    [ELEMENT_TYPE_LIGHTNING] = 10.0f,
};

static float burst_mana_amount[] = {
    [ELEMENT_TYPE_NONE] = 20.0f,
    [ELEMENT_TYPE_FIRE] = 20.0f,
    [ELEMENT_TYPE_ICE] = 20.0f,
    [ELEMENT_TYPE_LIGHTNING] = 20.0f,
};

static float contact_damage[] = {
    [ELEMENT_TYPE_NONE] = 0.0f,
    [ELEMENT_TYPE_FIRE] = 1.0f,
    [ELEMENT_TYPE_ICE] = 1.0f,
    [ELEMENT_TYPE_LIGHTNING] = 2.0f,
};

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options, enum element_type push_mode) {
    push->data_source = source;
    push->push_mode = push_mode;

    spell_data_source_retain(source);
    mana_regulator_init(&push->mana_regulator, event_options.burst_mana, 8.0f);

    push->dash_trail_right = dash_trail_new(&source->position, false);
    push->dash_trail_left = dash_trail_new(&source->position, true);

    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (target && HAS_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE) && push_mode == ELEMENT_TYPE_LIGHTNING) {
        push->should_restore_tangible = true;
        CLEAR_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE);
    } else {
        push->should_restore_tangible = false;
    }
}

void push_destroy(struct push* push) {
    if (push->should_restore_tangible) {
        struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

        if (target) {
            SET_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE);
        }
    }
    
    spell_data_source_release(push->data_source);

    if (push->dash_trail_right) {
        dash_trail_move(push->dash_trail_right, NULL);
    }
    if (push->dash_trail_left) {
        dash_trail_move(push->dash_trail_left, NULL);
    }
}

bool push_update(struct push* push, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    bool is_bursty = is_burst_dash[push->push_mode] || push->data_source->flags.cast_state == SPELL_CAST_STATE_INSTANT;

    if (!target) {
        return false;
    }

    if (contact_damage[push->push_mode]) {
        health_apply_contact_damage(target, contact_damage[push->push_mode], health_determine_damage_type(push->push_mode));
    }

    if (is_bursty && push->mana_regulator.burst_mana_rate == 0.0f) {
        float burst_mana = mana_pool_request(&spell_sources->mana_pool, burst_mana_amount[push->push_mode]);

        if (!burst_mana) {
            return false;
        }

        mana_regulator_init(&push->mana_regulator, burst_mana, 16.0f);
    }

    float power_ratio = mana_regulator_request(
        &push->mana_regulator, 
        is_bursty ? NULL : &spell_sources->mana_pool, 
        mana_per_second[push->push_mode] * scaled_time_step
    ) * scaled_time_step_inv * (1.0f / mana_per_second[push->push_mode]);

    if (power_ratio == 0.0f) {
        return false;
    }

    struct Vector3 targetVelocity;
    vector3Scale(&push->data_source->direction, &targetVelocity, push_strength[push->push_mode] * power_ratio);
    vector3MoveTowards(&target->velocity, &targetVelocity, scaled_time_step * 60.0f * power_ratio, &target->velocity);

    if (push->dash_trail_right) {
        dash_trail_move(push->dash_trail_right, target->position);
    }
    if (push->dash_trail_left) {
        dash_trail_move(push->dash_trail_left, target->position);
    }

    return true;
}