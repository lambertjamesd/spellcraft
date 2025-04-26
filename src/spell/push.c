#include "push.h"

#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "../entity/health.h"
#include "../util/flags.h"
#include "assets.h"

static struct push_definition push_definitions[] = {
    [ELEMENT_TYPE_NONE] = {},
    [ELEMENT_TYPE_FIRE] = {
        .push_strength = 4.0f,
        .mana_per_second = 10.0f,
        .burst_mana_amount = 20.0f,
        .contact_damage = 1.0f,
        .push_acceleration = 60.0f,
        .is_burst_dash = true,
        .ignore_gravity = true,
        .damage_type = DAMAGE_TYPE_FIRE,
    },
    [ELEMENT_TYPE_ICE] = {
        .push_strength = 16.0f,
        // .mana_per_second = 10.0f,
        .burst_mana_amount = 20.0f,
        .contact_damage = 1.0f,
        .push_acceleration = 18.0f,
        .is_burst_dash = false,
        .ignore_gravity = false,
        .damage_type = DAMAGE_TYPE_ICE,
    },
    [ELEMENT_TYPE_LIGHTNING] = {
        .push_strength = 10.0f,
        .mana_per_second = 10.0f,
        .burst_mana_amount = 20.0f,
        .contact_damage = 1.0f,
        .push_acceleration = 60.0f,
        .is_burst_dash = true,
        .ignore_gravity = false,
        .damage_type = DAMAGE_TYPE_LIGHTING,
    },
    [ELEMENT_TYPE_WATER] = {
        .push_strength = 8.0f,
        .mana_per_second = 4.0f,
        .burst_mana_amount = 8.0f,
        .contact_damage = 1.0f,
        .push_acceleration = 60.0f,
        .is_burst_dash = false,
        .ignore_gravity = false,
        .damage_type = DAMAGE_TYPE_WATER,
    },
};

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options, enum element_type push_mode) {
    push->data_source = source;
    push->definition = &push_definitions[push_mode];

    spell_data_source_retain(source);
    mana_regulator_init(&push->mana_regulator, event_options.burst_mana, 8.0f);

    push->dash_trail_right = dash_trail_new(&source->position, false);
    push->dash_trail_left = dash_trail_new(&source->position, true);

    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (target) {
        if (push->definition->damage_type == DAMAGE_TYPE_ICE) {
            target->disable_friction += 1;
        }
    }

    if (target && HAS_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE) && push_mode == ELEMENT_TYPE_LIGHTNING) {
        push->should_restore_tangible = true;
        CLEAR_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE);
    } else {
        push->should_restore_tangible = false;
    }
}

void push_destroy(struct push* push) {
    struct dynamic_object* target = collision_scene_find_object(push->data_source->target);

    if (target) {
        if (push->definition->damage_type == DAMAGE_TYPE_ICE) {
            target->disable_friction -= 1;
        }
    }

    if (push->should_restore_tangible && target) {
        SET_FLAG(target->collision_layers, COLLISION_LAYER_LIGHTING_TANGIBLE);
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

    bool is_bursty = push->definition->is_burst_dash || push->data_source->flags.cast_state == SPELL_CAST_STATE_INSTANT;

    if (!target) {
        return false;
    }

    if (push->definition->contact_damage) {
        health_apply_contact_damage(target, push->definition->contact_damage, health_determine_damage_type(push->definition->damage_type));
    }

    if (is_bursty && push->mana_regulator.burst_mana_rate == 0.0f) {
        float burst_mana = mana_pool_request(&spell_sources->mana_pool, push->definition->burst_mana_amount);

        if (!burst_mana) {
            return false;
        }

        mana_regulator_init(&push->mana_regulator, burst_mana, 16.0f);
    }

    float power_ratio;
    
    if (push->definition->mana_per_second) {
        power_ratio = mana_regulator_request(
            &push->mana_regulator, 
            is_bursty ? NULL : &spell_sources->mana_pool, 
            push->definition->mana_per_second * scaled_time_step
        ) * scaled_time_step_inv * (1.0f / push->definition->mana_per_second);
    } else {
        power_ratio = 1.0f;
    }

    if (power_ratio == 0.0f) {
        return false;
    }

    DYNAMIC_OBJECT_MARK_PUSHED(target);
    struct Vector3 targetVelocity;
    vector3Scale(&push->data_source->direction, &targetVelocity, push->definition->push_strength * power_ratio);
    if (!push->definition->ignore_gravity) {
        targetVelocity.y = target->velocity.y;
    }
    vector3MoveTowards(&target->velocity, &targetVelocity, scaled_time_step * push->definition->push_acceleration * power_ratio, &target->velocity);

    if (push->dash_trail_right) {
        dash_trail_move(push->dash_trail_right, target->position);
    }
    if (push->dash_trail_left) {
        dash_trail_move(push->dash_trail_left, target->position);
    }

    return push->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}