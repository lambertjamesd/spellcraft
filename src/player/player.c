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

static struct Vector2 player_max_rotation;

static struct dynamic_object_type player_visual_shape = {
    .minkowsi_sum = cylinder_minkowski_sum,
    .bounding_box = cylinder_bounding_box,
    .data = {
        .cylinder = {
            .half_height = 0.5f,
            .radius = 0.75f,
        }
    }
};

static struct cutscene_actor_def player_actor_def = {
    .eye_level = 1.26273f,
    .move_speed = 2.0f,
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

void player_run_clip(struct player* player, struct animation_clip* clip) {
    animator_run_clip(&player->cutscene_actor.animator, clip, 0.0f, false);
    player->cutscene_actor.animate_speed = 1.0f;
}

bool player_is_running(struct player* player, struct animation_clip* clip) {
    return animator_is_running_clip(&player->cutscene_actor.animator, clip);
}

void player_loop_animation(struct player* player, struct animation_clip* clip, float speed) {
    if (!animator_is_running_clip(&player->cutscene_actor.animator, clip)) {
        animator_run_clip(&player->cutscene_actor.animator, clip, 0.0f, true);
    }
    player->cutscene_actor.animate_speed = speed;
}

void player_handle_interaction(void* data, struct dynamic_object* overlaps) {
    bool* did_interact = (bool*)data;

    if (*did_interact) {
        return;
    }

    struct interactable* interactable = interactable_get(overlaps->entity_id);

    if (!interactable) {
        return;
    }
    
    *did_interact = true;
    interactable->callback(interactable, ENTITY_ID_PLAYER);
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

void player_handle_ground_movement(struct player* player, struct contact* ground_contact) {
    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    if (dynamic_object_should_slide(player_actor_def.collider.max_stable_slope, ground_contact->normal.y, ground_contact->surface_type)) {
        // TODO handle sliding logic
        return;
    }

    player_look_towards(player, &target_direction);

    if (player->cutscene_actor.collider.is_pushed) {
        return;
    }

    if (vector3MagSqrd(&target_direction) < 0.001f) {
        player->cutscene_actor.collider.velocity = gZeroVec;
    } else {
        struct Vector3 projected_target_direction;
        vector3ProjectPlane(&target_direction, &ground_contact->normal, &projected_target_direction);

        struct Vector3 normalized_direction;
        vector3Normalize(&target_direction, &normalized_direction);
        struct Vector3 projected_normalized;
        vector3ProjectPlane(&normalized_direction, &ground_contact->normal, &projected_normalized);

        vector3Scale(&projected_target_direction, &projected_target_direction, 1.0f / sqrtf(vector3MagSqrd(&projected_normalized)));

        vector3Scale(&projected_target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    }
}

void player_handle_air_movement(struct player* player) {
    if (player->cutscene_actor.collider.is_pushed) {
        return;
    }

    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    player_look_towards(player, &target_direction);

    float prev_y = player->cutscene_actor.collider.velocity.y;
    vector3Scale(&target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    player->cutscene_actor.collider.velocity.y = prev_y;
}

bool player_handle_a_action(struct player* player) {
    struct Vector3 query_center = player->cutscene_actor.transform.position;
    struct Vector3 query_offset;
    vector2ToLookDir(&player->cutscene_actor.transform.rotation, &query_offset);
    vector3AddScaled(&query_center, &query_offset, 1.0f, &query_center);
    query_center.y += player_visual_shape.data.cylinder.half_height;
    bool did_interact = false;
    collision_scene_query(&player_visual_shape, &query_center, COLLISION_LAYER_TANGIBLE, player_handle_interaction, &did_interact);
    return did_interact;
}

bool player_check_for_casting(struct player* player) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    struct spell_data_source* source = &player->player_spell_sources[4];
    
    if (source->flags.is_animating) {
        return true;
    }

    if (live_cast_has_pending_spell(&player->live_cast) && pressed.a) {
        spell_exec_start(&player->spell_exec, 4, live_cast_extract_active_spell(&player->live_cast), source);

        if (source->request_animation) {
            source->flags.is_animating = 1;

            struct animation_clip* to_play = NULL;
            switch (source->request_animation) {
                case SPELL_ANIMATION_SWING:
                    to_play = player->animations.swing_attack;
                    break;
                case SPELL_ANIMATION_SPIN:
                    to_play = player->animations.spin_attack;
                    break;
            }
            
            if (to_play) {
                player_run_clip(player, to_play);
            }
            player->last_spell_animation = to_play;
            source->request_animation = 0;
        }

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
        player_run_clip(player, player->animations.jump_peak);
    } else if (ground_contact) {
        player->state = PLAYER_GROUNDED;
        player_run_clip(player, player->animations.land);
    }
}

void player_update_falling(struct player* player, struct contact* ground_contact) {
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    player_check_for_casting(player);

    player_handle_air_movement(player);

    if (ground_contact) {
        player->state = PLAYER_GROUNDED;
        player_run_clip(player, player->animations.land);
    } else if (collider->under_water) {
        player->state = PLAYER_SWIMMING;
        player_loop_animation(player, player->animations.tread_water, 1.0f);
    } else if (!player_is_running(player, player->animations.jump_peak)) {
        player_loop_animation(player, player->animations.fall, 1.0f);
    }
}

void player_update_swimming(struct player* player, struct contact* ground_contact) {
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    player_handle_air_movement(player);

    if (ground_contact) {
        player->state = PLAYER_GROUNDED;
        player_run_clip(player, player->animations.land);
        return;
    } else if (!collider->under_water) {
        player->state = PLAYER_FALLING;
        player_loop_animation(player, player->animations.fall, 1.0f);
        return;
    }

    struct Vector3 horizontal_velocity = collider->velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    player_loop_animation(player, speed > 0.1f ? player->animations.swim : player->animations.tread_water, 1.0f);
}

void player_getting_up(struct player* player, struct contact* ground_contact) {
    if (!player_is_running(player, player->animations.knockback_land)) {
        player->state = PLAYER_GROUNDED;
    }
}

void player_update_knockback(struct player* player, struct contact* ground_contact) {
    if (ground_contact && player->cutscene_actor.collider.velocity.y < 0.1f) {
        player_run_clip(player, player->animations.knockback_land);
        player->state = PLAYER_GETTING_UP;
        return;
    }

    struct Vector2 target_rotation;
    vector2LookDir(&target_rotation, &player->cutscene_actor.collider.velocity);

    if (vector2MagSqr(&target_rotation) > 0.5f) {
        vector2Negate(&target_rotation, &target_rotation);
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &target_rotation, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }

    if (!player_is_running(player, player->animations.knocked_back)) {
        player_loop_animation(player, player->animations.knockback_fly, 1.0f);
    }
}

void player_update_grounded(struct player* player, struct contact* ground_contact) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    bool should_cast = true;

    if (pressed.a && !live_cast_is_typing(&player->live_cast) && player_handle_a_action(player)) {
        should_cast = false;
    }

    if (should_cast) {
        player_check_for_casting(player);
    }

    if (player->last_spell_animation && animator_is_running_clip(&player->cutscene_actor.animator, player->last_spell_animation)) {
        return;
    }

    if (ground_contact) {
        player_handle_ground_movement(player, ground_contact);
    } else {
        player_run_clip(player, player->animations.jump_peak);
        player->state = PLAYER_FALLING;
        return;
    }

    if (player_is_running(player, player->animations.take_damage)) {
        return;
    }

    for (int i = 0; i < 4; i += 1) {
        if (player->player_spell_sources[i].flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
            player_loop_animation(player, player->animations.attack_hold, 1.0f);
            return;
        }
    }

    struct Vector3 horizontal_velocity = collider->velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    if (player_is_running(player, player->animations.land)) {
        return;
    }

    if (collider->is_jumping) {
        player->state = PLAYER_JUMPING;
        player_run_clip(player, player->animations.jump);
        return;
    }

    if (collider->under_water) {
        player->state = PLAYER_SWIMMING;
        player_loop_animation(player, player->animations.tread_water, 1.0f);
        return;
    }

    if (collider->is_pushed) {
        if (ground_contact) {
            player_loop_animation(player, player->animations.dash, speed * (1.0f / PLAYER_MAX_SPEED));
            return;
        } else {
            player_loop_animation(player, player->animations.air_dash, 1.0f);
            return;
        }
    }

    if (speed < 0.2f) {
        player_loop_animation(player, player->animations.idle, 1.0f);
        return;
    }

    if (speed < PLAYER_RUN_THRESHOLD) {
        player_loop_animation(player, player->animations.walk, speed * (1.0f / PLAYER_WALK_ANIM_SPEED));
        return;
    }

    if (speed < PLAYER_DASH_THRESHOLD) {
        player_loop_animation(player, player->animations.run, speed * (1.0f / PLAYER_MAX_SPEED));
        return;
    }

    player_loop_animation(player, player->animations.dash, speed * (1.0f / PLAYER_MAX_SPEED));
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

void player_update(struct player* player) {
    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.d_up) {
        debug_collider_enable();
    } else if (pressed.d_down) {
        debug_collider_disable();
    }

    player_update_spells(player, input, pressed);
    player_check_collectables(player);
    player_check_inventory(player);

    if (cutscene_actor_update(&player->cutscene_actor) || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    struct contact* ground = dynamic_object_get_ground(&player->cutscene_actor.collider);
    player_update_state(player, ground);

    live_cast_cleanup_unused_spells(&player->live_cast, &player->spell_exec);
}

void player_knockback(struct player* player) {
    player->state = PLAYER_KNOCKBACK;
    player_run_clip(player, player->animations.knocked_back);
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
        animator_run_clip(&player->cutscene_actor.animator, player->animations.take_damage, 0.0f, false);
    }
    
    return damage->amount;
}

void player_load_animation(struct player* player) {
    player->animations.attack = animation_set_find_clip(player->cutscene_actor.animation_set, "attack1");
    player->animations.attack_hold = animation_set_find_clip(player->cutscene_actor.animation_set, "attack1_hold");
    player->animations.idle = animation_set_find_clip(player->cutscene_actor.animation_set, "idle");
    player->animations.run = animation_set_find_clip(player->cutscene_actor.animation_set, "run");
    player->animations.walk = animation_set_find_clip(player->cutscene_actor.animation_set, "walk");
    player->animations.dash = animation_set_find_clip(player->cutscene_actor.animation_set, "dash");
    player->animations.air_dash = animation_set_find_clip(player->cutscene_actor.animation_set, "air_dash");
    player->animations.take_damage = animation_set_find_clip(player->cutscene_actor.animation_set, "take_damage");

    player->animations.tread_water = animation_set_find_clip(player->cutscene_actor.animation_set, "tread_water");
    player->animations.swim = animation_set_find_clip(player->cutscene_actor.animation_set, "swim");

    player->animations.jump = animation_set_find_clip(player->cutscene_actor.animation_set, "jump");
    player->animations.jump_peak = animation_set_find_clip(player->cutscene_actor.animation_set, "jump_peak");
    player->animations.fall = animation_set_find_clip(player->cutscene_actor.animation_set, "fall");
    player->animations.land = animation_set_find_clip(player->cutscene_actor.animation_set, "land");

    player->animations.knocked_back = animation_set_find_clip(player->cutscene_actor.animation_set, "knocked_back");
    player->animations.knockback_fly = animation_set_find_clip(player->cutscene_actor.animation_set, "knockback_fly");
    player->animations.knockback_land = animation_set_find_clip(player->cutscene_actor.animation_set, "knockback_land");

    player->animations.swing_attack = animation_set_find_clip(player->cutscene_actor.animation_set, "swing_attack_0");
    player->animations.spin_attack = animation_set_find_clip(player->cutscene_actor.animation_set, "spin_attack");
}

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform) {
    transformSaInitIdentity(&player->cutscene_actor.transform);
    renderable_single_axis_init(&player->renderable, &player->cutscene_actor.transform, "rom:/meshes/characters/apprentice.tmesh");

    player->camera_transform = camera_transform;

    player->cutscene_actor.transform.position = definition->location;
    player->cutscene_actor.transform.rotation = definition->rotation;

    render_scene_add_renderable(&player->renderable, 2.0f);
    update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    vector2ComplexFromAngle(fixed_time_step * 7.0f, &player_max_rotation);

    struct TransformSingleAxis transform = {
        .position = definition->location,
        .rotation = definition->rotation,
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
    player->cutscene_actor.collider.weight_class = 1;

    spell_exec_init(&player->spell_exec);
    mana_pool_set_entity_id(&player->spell_exec.spell_sources.mana_pool, ENTITY_ID_PLAYER);
    live_cast_init(&player->live_cast);
    health_init(&player->health, ENTITY_ID_PLAYER, 100.0f);
    health_set_callback(&player->health, player_on_damage, player);

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

    tmesh_cache_release(player->assets.staffs[0]);
    drop_shadow_destroy(&player->drop_shadow);
}

struct Vector3* player_get_position(struct player* player) {
    return &player->cutscene_actor.transform.position;
}