#include "player.h"

#include <libdragon.h>

#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../collision/shapes/cylinder.h"
#include "../entity/interactable.h"
#include "../math/vector2.h"
#include "../objects/collectable.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"
#include "../debug/debug_colliders.h"
#include "../render/defs.h"

#include "../effects/fade_effect.h"

#define PLAYER_MAX_SPEED    4.2f

#define PLAYER_DASH_THRESHOLD 4.7f
#define PLAYER_RUN_THRESHOLD 1.4f
#define PLAYER_RUN_ANIM_SPEED   4.2f
#define PLAYER_WALK_ANIM_SPEED   0.64f

#define SLIDE_DELAY 0.25f
#define COYOTE_TIME 0.1f
#define SHADOW_AS_GROUND_DISTANCE   0.15f
#define GRAB_RADIUS 0.85f

#define CARRY_GRAB_TIME   (11.0f / 30.0f)
#define CARRY_DROP_TIME   (11.0f / 30.0f)

static struct Vector2 player_max_rotation;
static struct Vector2 z_target_rotation;

static struct spatial_trigger_type player_visual_shape = {
    SPATIAL_TRIGGER_WEDGE(2.0f, 1.0f, 0.707f, 0.707f),
    .center = {0.0f, 0.25f, 0.0f},
};

static struct spatial_trigger_type player_z_trigger_shape = {
    SPATIAL_TRIGGER_WEDGE(15.0f, 7.0f, 0.707f, 0.707f),
};

struct climb_up_data {
    float max_climb_height;
    float animation_height;
    float start_jump_time;
    float end_jump_time;
};

typedef struct climb_up_data climb_up_data_t;

static struct climb_up_data climb_up_data[CLIMB_UP_COUNT] = {
    {
        .max_climb_height = 0.8f,
        .animation_height = 0.4f,
        .start_jump_time = 14.0f / 30.0f,
        .end_jump_time = 23.0f / 30.0f,
    },
    {
        .max_climb_height = 1.2f,
        .animation_height = 0.9f,
        .start_jump_time = 6.0f / 30.0f,
        .end_jump_time = 13.0f / 30.0f,
    },
    {
        .max_climb_height = 1.8f,
        .animation_height = 1.4f,
        .start_jump_time = 6.0f / 30.0f,
        .end_jump_time = 13.0f / 30.0f,
    },
};

static struct cutscene_actor_def player_actor_def = {
    .eye_level = 1.26273f,
    .move_speed = PLAYER_WALK_ANIM_SPEED,
    .run_speed = PLAYER_RUN_ANIM_SPEED,
    .run_threshold = PLAYER_RUN_THRESHOLD,
    .rotate_speed = 2.0f,
    .half_height = 0.75f,
    .collision_layers = COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_PLAYER,
    .collision_group = COLLISION_GROUP_PLAYER,
    .collider = {
        .minkowsi_sum = capsule_minkowski_sum,
        .bounding_box = capsule_bounding_box,
        .data = {
            .capsule = {
                .radius = 0.25f,
                .inner_half_height = 0.5f,
            }
        },
        // about a 40 degree slope
        .max_stable_slope = 0.219131191f,
        .friction = 0.2f,
    },
};

void player_get_move_basis(struct Transform* transform, struct Vector3* forward, struct Vector3* right) {
    quatMultVector(&transform->rotation, &gForward, forward);
    quatMultVector(&transform->rotation, &gRight, right);

    if (forward->y > 0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
        vector3Negate(forward, forward);
    } else if (forward->y < -0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
    }

    forward->y = 0.0f;
    right->y = 0.0f;

    vector3Normalize(forward, forward);
    vector3Normalize(right, right);
}

void player_run_clip(struct player* player, enum player_animation clip) {
    animator_run_clip(&player->cutscene_actor.animator, player->animations[clip], 0.0f, false);
    player->cutscene_actor.animate_speed = 1.0f;
}

void player_run_clip_keep_translation(struct player* player, enum player_animation clip) {
    struct Transform before;
    armature_bone_transform(player->cutscene_actor.armature, 0, &before);
    player_run_clip(player, clip);
    animator_update(&player->cutscene_actor.animator, player->cutscene_actor.armature, fixed_time_step);
    struct Transform after;
    armature_bone_transform(player->cutscene_actor.armature, 0, &after);

    struct Vector3 final_point;
    transformSaTransformPoint(&player->cutscene_actor.transform, &before.position, &final_point);
    vector3AddScaled(
        &player->cutscene_actor.transform.position, 
        &final_point, 
        1.0f / MODEL_SCALE, 
        &player->cutscene_actor.transform.position
    );
    
    transformSaTransformPoint(&player->cutscene_actor.transform, &after.position, &final_point);
    vector3AddScaled(
        &player->cutscene_actor.transform.position, 
        &final_point, 
        -1.0f / MODEL_SCALE,
        &player->cutscene_actor.transform.position
    );
}

bool player_is_running(struct player* player, enum player_animation clip) {
    return animator_is_running_clip(&player->cutscene_actor.animator, player->animations[clip]);
}

void player_loop_animation(struct player* player, enum player_animation clip, float speed) {
    if (!animator_is_running_clip(&player->cutscene_actor.animator, player->animations[clip])) {
        animator_run_clip(&player->cutscene_actor.animator, player->animations[clip], 0.0f, true);
    }
    player->cutscene_actor.animate_speed = speed;
}

bool player_interact_with_entity(player_t* player,  entity_id entity) {
    interactable_t* interactable = interactable_get(entity);

    if (!interactable) {
        return false;
    }

    if (interactable->flags.grabbable) {
        dynamic_object_t* obj = collision_scene_find_object(entity);

        if (!obj) {
            return false;
        }

        struct Vector3 offset;
        vector3Sub(obj->position, player_get_position(player), &offset);

        if (vector3MagSqrd2D(&offset) > GRAB_RADIUS * GRAB_RADIUS) {
            return false;
        }

        float carry_offset = obj->position->y - obj->bounding_box.min.y;

        player->state = PLAYER_CARRY;
        player->state_data = (union state_data) {
            .carrying = {
                .carrying = entity,
                .carry_offset = carry_offset,
                .should_carry = false,
            }
        };
        player_run_clip(player, PLAYER_ANIMATION_CARRY_PICKUP);
        return true;
    } else {
        return interactable->callback(interactable, ENTITY_ID_PLAYER);
    }
}

struct player_interaction {
    struct player* player;
    bool did_interact;
};

void player_handle_interaction(void* data, struct dynamic_object* overlaps) {
    struct player_interaction* interaction = (struct player_interaction*)data;

    if (interaction->did_interact) {
        return;
    }
    
    interaction->did_interact = player_interact_with_entity(interaction->player, overlaps->entity_id);
}

void player_get_input_direction(struct player* player, struct Vector3* target_direction) {
    joypad_inputs_t input = joypad_get_inputs(0);

    struct Vector3 right;
    struct Vector3 forward;

    player_get_move_basis(player->camera_transform, &forward, &right);

    struct Vector2 direction;

    direction.x = input.stick_x * (1.0f / 80.0f);
    direction.y = -input.stick_y * (1.0f / 80.0f);

    float magSqrd = vector2MagSqr(&direction);

    if (magSqrd > 1.0f) {
        vector2Scale(&direction, 1.0f / sqrtf(magSqrd), &direction);
    }

    vector3Scale(&right, target_direction, direction.x);
    vector3AddScaled(target_direction, &forward, direction.y, target_direction);
}

void player_look_towards(struct player* player, struct Vector3* target_direction) {
    if (vector3MagSqrd(target_direction) > 0.01f) {
        struct Vector2 directionUnit;
        vector2LookDir(&directionUnit, target_direction);
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &directionUnit, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }
}

void player_handle_look(struct player* player, struct Vector3* look_direction) {
    if (player->z_target) {
        dynamic_object_t* obj = collision_scene_find_object(player->z_target);

        if (obj) {
            struct Vector3 offset;
            vector3Sub(obj->position, &player->cutscene_actor.transform.position, &offset);
            player_look_towards(player, &offset);
            return;
        }
    }

    player_look_towards(player, look_direction);
}

bool player_check_grab(struct player* player, struct Vector3* target_direction) {
    if (grab_checker_update(&player->grab_checker, &player->cutscene_actor.collider, target_direction)) {
        struct Vector3 target;
        grab_checker_get_climb_to(&player->grab_checker, &target);

        float height = target.y - player->cutscene_actor.transform.position.y;

        for (int i = 0; i < CLIMB_UP_COUNT; i += 1) {
            climb_up_data_t* data = &climb_up_data[i];

            if (height < data->max_climb_height) {
                player_run_clip(player, PLAYER_ANIMATION_CLIMB_UP_0 + i);
                player->state = PLAYER_CLIMBING_UP;
                player->state_data.climbing_up.timer = 0.0f;
                player->state_data.climbing_up.start_pos = player->cutscene_actor.transform.position;
                player->state_data.climbing_up.climb_up_index = i;
                player->state_data.climbing_up.y_velocity = (height - data->animation_height) / (data->end_jump_time - data->start_jump_time); 
                
                struct Vector3 offset;
                vector3Sub(&target, &player->cutscene_actor.transform.position, &offset);
                vector2LookDir(&player->state_data.climbing_up.target_rotation, &offset);

                return true;
            }
        }
    }
    
    return false;
}

void player_enter_grounded_state(struct player* player) {
    player->coyote_time = 0.0f;
    player->state = PLAYER_GROUNDED;
}

bool player_handle_ground_movement(struct player* player, struct contact* ground_contact, struct Vector3* target_direction, float* speed) {
    contact_t fake_contact = {
        .normal = gUp,
        .point = player->cutscene_actor.transform.position,
        .surface_type = SURFACE_TYPE_COYOTE,
    };
    
    *speed = sqrtf(vector3MagSqrd2D(&player->cutscene_actor.collider.velocity));

    if (ground_contact) {
        player->coyote_time = 0.0f;
    } else if (player->coyote_time < COYOTE_TIME) {
        player->coyote_time += fixed_time_step;
        ground_contact = &fake_contact;
    } else {
        return false;
    }

    if (dynamic_object_should_slide(player_actor_def.collider.max_stable_slope, ground_contact->normal.y, ground_contact->surface_type)) {
        // TODO handle sliding logic
        player->slide_timer += fixed_time_step;

        if (player->slide_timer > SLIDE_DELAY) {
            return true;
        }
    } else {
        player->slide_timer = 0.0f;
    }

    player_handle_look(player, target_direction);

    if (player->cutscene_actor.collider.is_pushed) {
        return true;
    }

    if (vector3MagSqrd(target_direction) < 0.001f) {
        player->cutscene_actor.collider.velocity = gZeroVec;
    } else {
        struct Vector3 projected_target_direction;
        vector3ProjectPlane(target_direction, &ground_contact->normal, &projected_target_direction);

        struct Vector3 normalized_direction;
        vector3Normalize(target_direction, &normalized_direction);
        struct Vector3 projected_normalized;
        vector3ProjectPlane(&normalized_direction, &ground_contact->normal, &projected_normalized);

        vector3Scale(&projected_target_direction, &projected_target_direction, 1.0f / sqrtf(vector3MagSqrd(&projected_normalized)));

        float prev_y = player->cutscene_actor.collider.velocity.y;

        vector3Scale(&projected_target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
        if (ground_contact->surface_type == SURFACE_TYPE_COYOTE) {
            player->cutscene_actor.collider.velocity.y = prev_y;
        }
    }

    if (ground_contact->other_object) {
        struct dynamic_object* ground_object = collision_scene_find_object(ground_contact->other_object);

        if (ground_object) {
            vector3Add(&player->cutscene_actor.collider.velocity, &ground_object->velocity, &player->cutscene_actor.collider.velocity);
        }
    } else if (ground_contact->surface_type != SURFACE_TYPE_COYOTE) {
        player->last_good_footing = player->cutscene_actor.transform.position;
    }

    return true;
}

void player_handle_air_movement(struct player* player) {
    if (player->cutscene_actor.collider.is_pushed) {
        return;
    }

    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    player_handle_look(player, &target_direction);

    float prev_y = player->cutscene_actor.collider.velocity.y;
    vector3Scale(&target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    player->cutscene_actor.collider.velocity.y = prev_y;
}

bool player_handle_a_action(struct player* player) {
    struct player_interaction interaction = {
        .player = player,
        .did_interact = false,
    };

    if (player->z_target) {
        if (player_interact_with_entity(player, player->z_target)) {
            return true;
        }
    }

    collision_scene_query_trigger(&player_visual_shape, &player->cutscene_actor.transform, COLLISION_LAYER_TANGIBLE, player_handle_interaction, &interaction);
    return interaction.did_interact;
}

void player_check_for_animation_request(struct player* player, struct spell_data_source* source) {
    if (source->request_animation) {
        source->flags.is_animating = 1;

        enum player_animation to_play = PLAYER_ANIMATION_COUNT;
        switch (source->request_animation) {
            case SPELL_ANIMATION_SWING:
                to_play = PLAYER_ANIMATION_SWING_ATTACK;
                break;
            case SPELL_ANIMATION_SPIN:
                to_play = PLAYER_ANIMATION_SPIN_ATTACK;
                break;
            case SPELL_ANIMATION_CAST_UP:
                to_play = PLAYER_ANIMATION_CAST_UP;
                break;
        }
        
        if (to_play != PLAYER_ANIMATION_COUNT) {
            player_run_clip(player, to_play);
            player->last_spell_animation = player->animations[to_play];
        } else {
            player->last_spell_animation = NULL;
        }
        source->request_animation = 0;
        player->player_spell_sources[4].flags.cast_state = SPELL_CAST_STATE_INACTIVE;
    }
}

bool player_check_for_casting(struct player* player) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    struct spell_data_source* source = &player->player_spell_sources[4];
    
    if (source->flags.is_animating) {
        return true;
    } else {
        player_check_for_animation_request(player, source);
    }

    if (live_cast_has_pending_spell(&player->live_cast) && pressed.a) {
        spell_exec_start(&player->spell_exec, 4, live_cast_use_spell(&player->live_cast), source);
        player_check_for_animation_request(player, source);
        return true;
    }

    if (pressed.a && spell_exec_charge(&player->spell_exec)) {
        return true;
    }

    return false;
}

void player_update_jumping(struct player* player, struct contact* ground_contact) {
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    if (collider->velocity.y < 0.0f) {
        player->state = PLAYER_FALLING;
        player_run_clip(player, PLAYER_ANIMATION_JUMP_PEAK);
    } else if (ground_contact) {
        player_enter_grounded_state(player);
        player_run_clip(player, PLAYER_ANIMATION_LAND);
    }
}

void player_update_falling(struct player* player, struct contact* ground_contact) {
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    player_check_for_casting(player);

    if (ground_contact) {
        player_enter_grounded_state(player);
        player_run_clip(player, PLAYER_ANIMATION_LAND);
        return;
    } else if (collider->under_water) {
        player->state = PLAYER_SWIMMING;
        player_loop_animation(player, PLAYER_ANIMATION_TREAD_WATER, 1.0f);
        return;
    } else if (!player_is_running(player, PLAYER_ANIMATION_JUMP_PEAK)) {
        player_loop_animation(player, PLAYER_ANIMATION_FALL, 1.0f);
    }
    
    player_handle_air_movement(player);
}

void player_update_swimming(struct player* player, struct contact* ground_contact) {
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    player_handle_air_movement(player);

    if (ground_contact) {
        player->state = PLAYER_GROUNDED;
        player_run_clip(player, PLAYER_ANIMATION_LAND);
        return;
    } else if (!collider->under_water) {
        player->state = PLAYER_FALLING;
        player_loop_animation(player, PLAYER_ANIMATION_FALL, 1.0f);
        return;
    }

    struct Vector3 horizontal_velocity = collider->velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    player_loop_animation(player, speed > 0.1f ? PLAYER_ANIMATION_SWIM : PLAYER_ANIMATION_TREAD_WATER, 1.0f);
}

void player_getting_up(struct player* player, struct contact* ground_contact) {
    if (!player_is_running(player, PLAYER_ANIMATION_KNOCKBACK_LAND)) {
        player->state = PLAYER_GROUNDED;
    }
}

void player_climbing_up(struct player* player, struct contact* ground_contact) {
    union state_data* state = &player->state_data;
    struct climb_up_data* climb_up = &climb_up_data[state->climbing_up.climb_up_index];

    if (state->climbing_up.timer > climb_up->end_jump_time && !player_is_running(player, PLAYER_ANIMATION_CLIMB_UP_0 + state->climbing_up.climb_up_index)) {
        player->state = PLAYER_GROUNDED;
        player_run_clip_keep_translation(player, PLAYER_ANIMATION_IDLE);
        return;
    }

    struct Vector3* pos = &player->cutscene_actor.transform.position;
    *pos = state->climbing_up.start_pos;
    player->cutscene_actor.collider.velocity = gZeroVec;

    if (state->climbing_up.timer > climb_up->end_jump_time) {
        pos->y += state->climbing_up.y_velocity * (climb_up->end_jump_time - climb_up->start_jump_time);
        player->cutscene_actor.collider.velocity.y = state->climbing_up.y_velocity;
    } else if (state->climbing_up.timer > climb_up->start_jump_time) {
        pos->y += state->climbing_up.y_velocity * (state->climbing_up.timer - climb_up->start_jump_time);
    } else {
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &state->climbing_up.target_rotation, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }

    player->state_data.climbing_up.timer += fixed_time_step;
}

void player_carry(player_t* player, contact_t* ground_contact) {
    state_data_t* state_data = (state_data_t*)&player->state_data;

    dynamic_object_t* obj = collision_scene_find_object(state_data->carrying.carrying);

    if (!obj) {
        player->state = ground_contact ? PLAYER_GROUNDED : PLAYER_FALLING;
        return;
    }

    if (state_data->carrying.should_carry) {
        transform_sa_t* player_transform = &player->cutscene_actor.transform;

        struct Transform cast_transform;
        armature_bone_transform(player->cutscene_actor.armature, player->renderable.mesh->attatchments[0].bone_index, &cast_transform);

        struct Vector3 position;
        vector3Scale(&cast_transform.position, &cast_transform.position, 1.0f / MODEL_SCALE);
        cast_transform.position.x = 0.0f;

        transformSaTransformPoint(player_transform, &cast_transform.position, &position);
        position.y += state_data->carrying.carry_offset;
        *obj->position = position;
        obj->velocity = player->cutscene_actor.collider.velocity;

        if (obj->rotation) {
            *obj->rotation = player_transform->rotation;
        }
    }

    if (player_is_running(player, PLAYER_ANIMATION_CARRY_PICKUP)) {
        if (animator_get_time(&player->cutscene_actor.animator) > CARRY_GRAB_TIME) {
            player->state_data.carrying.should_carry = true;
            obj->is_fixed = true;
            obj->weight_class = WEIGHT_CLASS_GHOST;
        }

        return;
    }

    if (player_is_running(player, PLAYER_ANIMATION_CARRY_DROP)) {
        if (animator_get_time(&player->cutscene_actor.animator) > CARRY_DROP_TIME) {
            player->state_data.carrying.should_carry = false;
            obj->is_fixed = false;
            obj->weight_class =  WEIGHT_CLASS_LIGHT;
        }

        return;
    }

    if (!state_data->carrying.should_carry) {
        player->state = ground_contact ? PLAYER_GROUNDED : PLAYER_FALLING;
        return;
    }

    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.a) {
        player_run_clip(player, PLAYER_ANIMATION_CARRY_DROP);
        return;
    }
    
    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    float speed;

    if (!player_handle_ground_movement(player, ground_contact, &target_direction, &speed)) {
        player_loop_animation(player, PLAYER_ANIMATION_CARRY_IDLE, 1.0f);
        return;
    }

    if (speed < 0.2f) {
        player_loop_animation(player, PLAYER_ANIMATION_CARRY_IDLE, 1.0f);
        return;
    }

    if (speed < PLAYER_RUN_THRESHOLD) {
        player_loop_animation(player, PLAYER_ANIMATION_CARRY_WALK, speed * (1.0f / PLAYER_WALK_ANIM_SPEED));
        return;
    }

    player_loop_animation(player, PLAYER_ANIMATION_CARRY_RUN, speed * (1.0f / PLAYER_MAX_SPEED));
}

void player_update_knockback(struct player* player, struct contact* ground_contact) {
    if (ground_contact && player->cutscene_actor.collider.velocity.y < 0.1f) {
        player_run_clip(player, PLAYER_ANIMATION_KNOCKBACK_LAND);
        player->state = PLAYER_GETTING_UP;
        return;
    }

    struct Vector2 target_rotation;
    vector2LookDir(&target_rotation, &player->cutscene_actor.collider.velocity);

    if (vector2MagSqr(&target_rotation) > 0.5f) {
        vector2Negate(&target_rotation, &target_rotation);
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &target_rotation, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }

    if (!player_is_running(player, PLAYER_ANIMATION_KNOCKED_BACK)) {
        player_loop_animation(player, PLAYER_ANIMATION_KNOCKBACK_FLY, 1.0f);
    }
}

void player_update_grounded(struct player* player, struct contact* ground_contact) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    bool should_cast = true;

    if (pressed.a && !live_cast_is_typing(&player->live_cast) && player_handle_a_action(player)) {
        should_cast = false;
    }

    if (player->state != PLAYER_GROUNDED) {
        return;
    }

    if (should_cast) {
        player_check_for_casting(player);
    }

    if (player->last_spell_animation && animator_is_running_clip(&player->cutscene_actor.animator, player->last_spell_animation)) {
        return;
    }
    
    if (collider->is_jumping) {
        player->state = PLAYER_JUMPING;
        player_run_clip(player, PLAYER_ANIMATION_JUMP);
        return;
    }

    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    if (player_check_grab(player, &target_direction)) {
        return;
    }

    float speed;
    if (!player_handle_ground_movement(player, ground_contact, &target_direction, &speed)) {
        player_run_clip(player, PLAYER_ANIMATION_JUMP_PEAK);
        player->state = PLAYER_FALLING;
        return;
    }

    if (player_is_running(player, PLAYER_ANIMATION_TAKE_DAMAGE)) {
        return;
    }

    for (int i = 0; i < 4; i += 1) {
        if (player->player_spell_sources[i].flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
            player_loop_animation(player, PLAYER_ANIMATION_ATTACK_HOLD, 1.0f);
            return;
        }
    }

    if (player_is_running(player, PLAYER_ANIMATION_LAND)) {
        return;
    }

    if (collider->under_water) {
        player->state = PLAYER_SWIMMING;
        player_loop_animation(player, PLAYER_ANIMATION_TREAD_WATER, 1.0f);
        return;
    }

    if (collider->is_pushed) {
        if (ground_contact) {
            player_loop_animation(player, PLAYER_ANIMATION_DASH, speed * (1.0f / PLAYER_MAX_SPEED));
            return;
        } else {
            player_loop_animation(player, PLAYER_ANIMATION_AIR_DASH, 1.0f);
            return;
        }
    }

    if (speed < 0.2f) {
        player_loop_animation(player, PLAYER_ANIMATION_IDLE, 1.0f);
        return;
    }

    if (speed < PLAYER_RUN_THRESHOLD) {
        player_loop_animation(player, PLAYER_ANIMATION_WALK, speed * (1.0f / PLAYER_WALK_ANIM_SPEED));
        return;
    }

    if (speed < PLAYER_DASH_THRESHOLD) {
        player_loop_animation(player, PLAYER_ANIMATION_RUN, speed * (1.0f / PLAYER_MAX_SPEED));
        return;
    }

    player_loop_animation(player, PLAYER_ANIMATION_DASH, speed * (1.0f / PLAYER_MAX_SPEED));
    return;
}

void player_update_state(struct player* player, struct contact* ground_contact) {
    switch (player->state) {
        case PLAYER_JUMPING:
            player_update_jumping(player, ground_contact);
            break;
        case PLAYER_FALLING:
            player_update_falling(player, ground_contact);
            break;
        case PLAYER_SWIMMING:
            player_update_swimming(player, ground_contact);
            break;
        case PLAYER_KNOCKBACK:
            player_update_knockback(player, ground_contact);
            break;
        case PLAYER_GETTING_UP:
            player_getting_up(player, ground_contact);
            break;
        case PLAYER_CLIMBING_UP:
            player_climbing_up(player, ground_contact);
            break;
        case PLAYER_CARRY:
            player_carry(player, ground_contact);
            break;
        default:
            player_update_grounded(player, ground_contact);
            break;
    }
}

bool player_cast_state(joypad_buttons_t buttons, int button_index) {
    switch (button_index) {
        case 0:
            return buttons.c_up;
        case 1:
            return buttons.c_down;
        case 2:
            return buttons.c_left;
        case 3:
            return buttons.c_right;
        default: 
            return false;
    }
}

void player_check_inventory(struct player* player) {
    struct staff_stats* staff = inventory_equipped_staff();
    player->renderable.attachments[0] = staff->item_type == ITEM_TYPE_NONE ? NULL : player->assets.staffs[staff->staff_index];
}

void player_update_spells(struct player* player, joypad_inputs_t input, joypad_buttons_t pressed) {
    struct Vector3 castDirection;
    vector2ToLookDir(&player->cutscene_actor.transform.rotation, &castDirection);

    // for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
    //     struct spell_data_source* source = &player->player_spell_sources[i];

    //     source->direction = castDirection;
    //     source->position = player->transform.position;
    //     source->position.y += 1.0f;
    //     source->flags.cast_state = player_cast_state(input.btn, i) ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;

    //     if (player_cast_state(pressed, i) && inventory_get_equipped_spell(i)) {
    //         spell_exec_start(&player->spell_exec, 0, inventory_get_equipped_spell(i), source);
    //     }
    // }

    // A cast slot
    struct spell_data_source* source = &player->player_spell_sources[4];

    if (source->flags.is_animating) {
        struct Transform cast_transform;
        armature_bone_transform(player->cutscene_actor.armature, player->renderable.mesh->attatchments[0].bone_index, &cast_transform);

        struct Vector3 direction;
        quatMultVector(&cast_transform.rotation, &gUp, &direction);
        vector3RotateWith2(&direction, &player->cutscene_actor.transform.rotation, &source->direction);

        vector3RotateWith2(&cast_transform.position, &player->cutscene_actor.transform.rotation, &direction);
        vector3AddScaled(&player->cutscene_actor.transform.position, &direction, 1.0f / MODEL_SCALE, &source->position);

        if (!animator_is_running_clip(&player->cutscene_actor.animator, player->last_spell_animation)) {
            source->flags.is_animating = 0;
            source->flags.cast_state = SPELL_CAST_STATE_INACTIVE;
        } else {
            source->flags.cast_state = player->cutscene_actor.animator.events ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;
        }
    } else {
        source->direction = castDirection;
        source->position = player->cutscene_actor.transform.position;
        source->position.y += 1.0f;
        source->flags.cast_state = input.btn.a ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;
    }

    if (pressed.b) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_LIFE);
    } else if (pressed.c_up) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_AIR);
    } else if (pressed.c_down) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_EARTH);
    } else if (pressed.c_right) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_FIRE);
    } else if (pressed.c_left) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_ICE);
    } else if (live_cast_has_pending_spell(&player->live_cast) && pressed.r) {
        live_cast_append_symbol(&player->live_cast, SPELL_SYMBOL_BREAK);
    }
}

void player_check_collectables(struct player* player) {
    struct contact* contact = player->cutscene_actor.collider.active_contacts;

    while (contact) {
        struct collectable* collectable = collectable_get(contact->other_object);

        if (collectable) {
            collectable_collected(collectable);
        }

        contact = contact->next;
    }
}

void player_find_z_target(struct player* player) {
    struct contact* contact = player->z_target_trigger.active_contacts;
    struct contact* nearest_target = NULL;
    float nearest_distance = 0.0f;

    while (contact) {
        float distance = vector3DistSqrd(&contact->point, &player->cutscene_actor.transform.position);

        if (nearest_target == NULL || distance < nearest_distance) {
            nearest_target = contact;
            nearest_distance = distance;
        }

        contact = contact->next;
    }

    if (nearest_target) {
        player->z_target = nearest_target->other_object;
    }
}

void player_handle_z_target(struct player* player, bool z_pressed, bool z_down) {
    if (z_pressed) {
        player_find_z_target(player);
    }

    if (!player->z_target || !z_down) {
        player->z_target = 0;
        player->z_target_visual.hide = 1;
        return;
    }

    struct dynamic_object* obj = collision_scene_find_object(player->z_target);

    if (!z_down || !obj) {
        player->z_target = 0;
        player->z_target_visual.hide = 1;
        player_find_z_target(player);
        return;
    }

    player->z_target_visual.hide = 0;
    player->z_target_transform.position = *obj->position;
    struct Vector3 top_of_target;
    dynamic_object_minkowski_sum(obj, &gUp, &top_of_target);
    player->z_target_transform.position.y = top_of_target.y + 0.25f;
    struct Vector2 new_rotation;
    vector2ComplexMul(&player->z_target_transform.rotation, &z_target_rotation, &new_rotation);
    player->z_target_transform.rotation = new_rotation;
}

void player_update(struct player* player) {
    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.d_up) {
        debug_collider_enable();
    } else if (pressed.d_down) {
        debug_collider_disable();
    }

    player_handle_z_target(player, pressed.z, input.btn.z);

    player_update_spells(player, input, pressed);
    player_check_collectables(player);
    player_check_inventory(player);

    if (cutscene_actor_update(&player->cutscene_actor) || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    struct contact* ground = dynamic_object_get_ground(&player->cutscene_actor.collider);
    player_update_state(player, ground);

    if (player->cutscene_actor.collider.hit_kill_plane) {
        player->cutscene_actor.transform.position = player->last_good_footing;
        player->cutscene_actor.collider.velocity = gZeroVec;
        player->cutscene_actor.collider.hit_kill_plane = 0;
    }
}

void player_knockback(struct player* player) {
    player->state = PLAYER_KNOCKBACK;
    player_run_clip(player, PLAYER_ANIMATION_KNOCKED_BACK);
}

float player_on_damage(void* data, struct damage_info* damage) {
    struct player* player = (struct player*)data;

    if (player->state == PLAYER_KNOCKBACK || 
        player->state == PLAYER_GETTING_UP ||
        cutscene_actor_is_moving(&player->cutscene_actor)) {
        return 0.0f;
    }

    if (damage->type & DAMAGE_TYPE_KNOCKBACK) {
        player_knockback(player);
    } else {
        player_run_clip(player, PLAYER_ANIMATION_TAKE_DAMAGE);
    }
    
    return damage->amount;
}

static const char* animation_clip_names[PLAYER_ANIMATION_COUNT] = {
    [PLAYER_ANIMATION_IDLE] = "idle",
    [PLAYER_ANIMATION_RUN] = "run",
    [PLAYER_ANIMATION_WALK] = "walk",
    [PLAYER_ANIMATION_DASH] = "dash",
    [PLAYER_ANIMATION_ATTACK] = "attack1",
    [PLAYER_ANIMATION_ATTACK_HOLD] = "attack1_hold",
    [PLAYER_ANIMATION_AIR_DASH] = "air_das",
    [PLAYER_ANIMATION_TAKE_DAMAGE] = "take_damage",
    
    [PLAYER_ANIMATION_TREAD_WATER] = "tread_water",
    [PLAYER_ANIMATION_SWIM] = "swim",
    
    [PLAYER_ANIMATION_JUMP] = "jump",
    [PLAYER_ANIMATION_JUMP_PEAK] = "jump_peak",
    [PLAYER_ANIMATION_FALL] = "fall",
    [PLAYER_ANIMATION_LAND] = "land",

    [PLAYER_ANIMATION_KNOCKED_BACK] = "knocked_back",
    [PLAYER_ANIMATION_KNOCKBACK_FLY] = "knockback_fly",
    [PLAYER_ANIMATION_KNOCKBACK_LAND] = "knockback_land",

    [PLAYER_ANIMATION_SWING_ATTACK] = "swing_attack_0",
    [PLAYER_ANIMATION_SPIN_ATTACK] = "spin_attack",
    [PLAYER_ANIMATION_CAST_UP] = "cast_up",
    
    [PLAYER_ANIMATION_CLIMB_UP_0] = "climb_0",
    [PLAYER_ANIMATION_CLIMB_UP_1] = "climb_1",
    [PLAYER_ANIMATION_CLIMB_UP_2] = "climb_2",
    
    [PLAYER_ANIMATION_CARRY_PICKUP] = "carry_pickup",
    [PLAYER_ANIMATION_CARRY_IDLE] = "carry_idle",
    [PLAYER_ANIMATION_CARRY_RUN] = "carry_run",
    [PLAYER_ANIMATION_CARRY_WALK] = "carry_walk",
    [PLAYER_ANIMATION_CARRY_DROP] = "carry_drop",
};

void player_load_animation(struct player* player) {
    for (int i = 0; i < PLAYER_ANIMATION_COUNT; i += 1) {
        player->animations[i] = animation_set_find_clip(player->cutscene_actor.animation_set, animation_clip_names[i]);
    }
}

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform) {
    transformSaInitIdentity(&player->cutscene_actor.transform);
    renderable_single_axis_init(&player->renderable, &player->cutscene_actor.transform, "rom:/meshes/characters/apprentice.tmesh");

    player->camera_transform = camera_transform;

    player->cutscene_actor.transform.position = definition->location;
    player->cutscene_actor.transform.rotation = definition->rotation;

    player->last_good_footing = definition->location;

    render_scene_add_renderable(&player->renderable, 2.0f);
    update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    vector2ComplexFromAngle(fixed_time_step * 7.0f, &player_max_rotation);
    vector2ComplexFromAngle(fixed_time_step * 2.0f, &z_target_rotation);

    struct TransformSingleAxis transform = {
        .position = definition->location,
        .rotation = definition->rotation,
        .scale = 1.0f,
    };

    cutscene_actor_init(
        &player->cutscene_actor,
        &player_actor_def,
        ENTITY_ID_PLAYER,
        &transform,
        NPC_TYPE_PLAYER,
        0,
        &player->renderable.armature,
        "rom:/meshes/characters/apprentice.anim"
    );

    player->cutscene_actor.collider.density_class = DYNAMIC_DENSITY_MEDIUM;
    player->cutscene_actor.collider.weight_class = WEIGHT_CLASS_MEDIUM;

    spell_exec_init(&player->spell_exec);
    mana_pool_set_entity_id(&player->spell_exec.spell_sources.mana_pool, ENTITY_ID_PLAYER);
    live_cast_init(&player->live_cast);
    health_init(&player->health, ENTITY_ID_PLAYER, 100.0f);
    health_set_callback(&player->health, player_on_damage, player);
    grab_checker_init(&player->grab_checker, &player_actor_def.collider);

    for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
        struct spell_data_source* source = &player->player_spell_sources[i];

        source->flags.all = 0;
        source->flags.has_animator = 1;
        source->reference_count = 1;
        source->request_animation = 0;
        source->target = ENTITY_ID_PLAYER;
    }

    player_load_animation(player);

    player->last_spell_animation = NULL;

    player->state = PLAYER_GROUNDED;

    player->assets.staffs[0] = tmesh_cache_load("rom:/meshes/objects/staff_default.tmesh");
    player->assets.staffs[1] = NULL;
    player->assets.staffs[2] = NULL;
    player->assets.staffs[3] = NULL;

    drop_shadow_init(&player->drop_shadow, &player->cutscene_actor.collider);

    spatial_trigger_init(&player->z_target_trigger, &player->cutscene_actor.transform, &player_z_trigger_shape, COLLISION_LAYER_Z_TARGET);
    collision_scene_add_trigger(&player->z_target_trigger);
    player->z_target_transform = player->cutscene_actor.transform;
    renderable_single_axis_init(&player->z_target_visual, &player->z_target_transform, "rom:/meshes/player/z_cursor.tmesh");
    render_scene_add_renderable(&player->z_target_visual, 1.0f);
    player->z_target = 0;
    player->z_target_visual.hide = 1;
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);
    spell_exec_destroy(&player->spell_exec);
    live_cast_destroy(&player->live_cast);
    health_destroy(&player->health);
    mana_pool_clear_entity_id(ENTITY_ID_PLAYER);

    render_scene_remove(&player->renderable);
    update_remove(player);
    cutscene_actor_destroy(&player->cutscene_actor);
    grab_checker_destroy(&player->grab_checker);

    tmesh_cache_release(player->assets.staffs[0]);
    drop_shadow_destroy(&player->drop_shadow);

    collision_scene_remove_trigger(&player->z_target_trigger);
    render_scene_remove(&player->z_target_visual);
    renderable_destroy(&player->z_target_visual);
}

struct Vector3* player_get_position(struct player* player) {
    return &player->cutscene_actor.transform.position;
}