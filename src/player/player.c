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
    .move_speed = 1.0f,
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

struct animation_clip* player_determine_animation_jumping(struct player* player, struct contact* ground_contact, float* playback_speed, bool* repeat) {
    struct animation_clip* current_clip = player->cutscene_actor.animator.current_clip;
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    *playback_speed = 1.0f;
    *repeat = false;
    if (collider->velocity.y < 0.0f) {
        player->animations.state = PLAYER_ANIMATION_FALLING;
        return player->animations.jump_peak;
    } else if (ground_contact) {
        player->animations.state = PLAYER_ANIMATION_GROUNDED;
        return player->animations.land;
    }

    return NULL;
}

struct animation_clip* player_determine_animation_falling(struct player* player, struct contact* ground_contact, float* playback_speed, bool* repeat) {
    struct animation_clip* current_clip = player->cutscene_actor.animator.current_clip;
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    *playback_speed = 1.0f;

    if (ground_contact) {
        *repeat = false;
        player->animations.state = PLAYER_ANIMATION_GROUNDED;
        return player->animations.land;
    } else if (collider->under_water) {
        player->animations.state = PLAYER_ANIMATION_SWIMMING;
        *repeat = true;
        return player->animations.tread_water;
    } else if (current_clip == player->animations.jump_peak) {
        *repeat = false;
        return player->animations.jump_peak;
    } else {
        *repeat = false;
        return player->animations.fall;
    }
}

struct animation_clip* player_determine_animation_swimming(struct player* player, struct contact* ground_contact, float* playback_speed, bool* repeat) {
    struct animation_clip* current_clip = player->cutscene_actor.animator.current_clip;
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    *playback_speed = 1.0f;

    if (ground_contact) {
        *repeat = false;
        player->animations.state = PLAYER_ANIMATION_GROUNDED;
        return player->animations.land;
    } else if (!collider->under_water) {
        player->animations.state = PLAYER_ANIMATION_FALLING;
        *repeat = true;
        return player->animations.fall;
    }

    struct Vector3 horizontal_velocity = collider->velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    *playback_speed = 1.0f;
    *repeat = true;
    return speed > 0.1f ? player->animations.swim : player->animations.tread_water;
}

struct animation_clip* player_determine_animation_grounded(struct player* player, struct contact* ground_contact, float* playback_speed, bool* repeat) {
    struct animation_clip* current_clip = player->cutscene_actor.animator.current_clip;
    struct dynamic_object* collider = &player->cutscene_actor.collider;

    
    if (current_clip == player->animations.take_damage && 
        animator_is_running(&player->cutscene_actor.animator)) {
        return player->animations.take_damage;
    }

    for (int i = 0; i < 4; i += 1) {
        if (player->player_spell_sources[i].flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
            *playback_speed = 1.0f;
            *repeat = false;
            return player->animations.attack_hold;
        }
    }

    struct Vector3 horizontal_velocity = collider->velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    if (current_clip == player->animations.land) {
        *playback_speed = 1.0f;
        *repeat = false;
        return player->animations.land;
    }

    if (collider->is_jumping) {
        *playback_speed = 1.0f;
        *repeat = false;
        player->animations.state = PLAYER_ANIMATION_JUMPING;
        return player->animations.jump;
    }

    if (collider->under_water) {
        *playback_speed = 1.0f;
        *repeat = true;
        player->animations.state = PLAYER_ANIMATION_SWIMMING;
        return player->animations.tread_water;
    }

    if (collider->is_pushed) {
        if (ground_contact) {
            *playback_speed = speed * (1.0f / PLAYER_MAX_SPEED);
            *repeat = true;
            return player->animations.dash;
        } else {
            *playback_speed = 1.0f;
            *repeat = true;
            return player->animations.air_dash;
        }
    }

    if (speed < 0.2f) {
        *playback_speed = 1.0f;
        *repeat = true;
        return player->animations.idle;
    }

    if (speed < PLAYER_RUN_THRESHOLD) {
        *playback_speed = speed * (1.0f / PLAYER_WALK_ANIM_SPEED);
        *repeat = true;
        return player->animations.walk;
    }

    if (speed < PLAYER_DASH_THRESHOLD) {
        *playback_speed = speed * (1.0f / PLAYER_MAX_SPEED);
        *repeat = true;
        return player->animations.run;
    }

    *playback_speed = speed * (1.0f / PLAYER_MAX_SPEED);
    *repeat = true;
    return player->animations.dash;
}

struct animation_clip* player_determine_animation(struct player* player, struct contact* ground_contact, float* playback_speed, bool* repeat) {
    if (player->animations.state == PLAYER_ANIMATION_JUMPING) {
        return player_determine_animation_jumping(player, ground_contact, playback_speed, repeat);
    }

    if (player->animations.state == PLAYER_ANIMATION_FALLING) {
        return player_determine_animation_falling(player, ground_contact, playback_speed, repeat);
    }

    if (player->animations.state == PLAYER_ANIMATION_SWIMMING) {
        return player_determine_animation_swimming(player, ground_contact, playback_speed, repeat);
    }

    return player_determine_animation_grounded(player, ground_contact, playback_speed, repeat);
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

void player_handle_interaction(void* data, struct dynamic_object* overlaps) {
    bool* did_intersect = (bool*)data;

    if (*did_intersect) {
        return;
    }

    struct interactable* interactable = interactable_get(overlaps->entity_id);

    if (!interactable) {
        return;
    }
    
    *did_intersect = true;
    interactable->callback(interactable, 0);
}

void player_handle_a_action(struct player* player) {
    if (spell_exec_charge(&player->spell_exec)) {
        return;
    }

    struct Vector3 query_center = player->cutscene_actor.transform.position;
    struct Vector3 query_offset;
    vector2ToLookDir(&player->cutscene_actor.transform.rotation, &query_offset);
    vector3AddScaled(&query_center, &query_offset, 1.0f, &query_center);
    query_center.y += player_visual_shape.data.cylinder.half_height;
    bool did_intersect = false;
    collision_scene_query(&player_visual_shape, &query_center, COLLISION_LAYER_TANGIBLE, player_handle_interaction, &did_intersect);
}

void player_check_inventory(struct player* player) {
    struct staff_stats* staff = inventory_equipped_staff();
    player->renderable.attachments[0] = staff->item_type == ITEM_TYPE_NONE ? NULL : player->assets.staffs[staff->staff_index];
}

void player_handle_ground_movement(struct player* player, struct contact* ground_contact, struct Vector3* target_direction) {
    if (dynamic_object_should_slide(player_actor_def.collider.max_stable_slope, ground_contact->normal.y, ground_contact->surface_type)) {
        // TODO handle sliding logic
        return;
    }

    if (player->cutscene_actor.collider.is_pushed) {
        return;
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

        vector3Scale(&projected_target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    }
}

void player_handle_air_movement(struct player* player, struct Vector3* target_direction) {
    if (player->cutscene_actor.collider.is_pushed) {
        return;
    }

    float prev_y = player->cutscene_actor.collider.velocity.y;
    vector3Scale(target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    player->cutscene_actor.collider.velocity.y = prev_y;
}

void player_handle_movement(struct player* player, joypad_inputs_t* input, struct contact* ground_contact) {
    struct Vector3 right;
    struct Vector3 forward;

    player_get_move_basis(player->camera_transform, &forward, &right);

    struct Vector2 direction;

    direction.x = input->stick_x * (1.0f / 80.0f);
    direction.y = -input->stick_y * (1.0f / 80.0f);

    float magSqrd = vector2MagSqr(&direction);

    if (magSqrd > 1.0f) {
        vector2Scale(&direction, 1.0f / sqrtf(magSqrd), &direction);
    }

    struct Vector3 target_direction;
    vector3Scale(&right, &target_direction, direction.x);
    vector3AddScaled(&target_direction, &forward, direction.y, &target_direction);

    if (ground_contact) {
        player_handle_ground_movement(player, ground_contact, &target_direction);
    } else {
        player_handle_air_movement(player, &target_direction);
    }

    if (magSqrd > 0.01f) {
        struct Vector2 directionUnit;
        vector2LookDir(&directionUnit, &target_direction);
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &directionUnit, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }
}

void player_update(struct player* player) {
    struct contact* ground = dynamic_object_get_ground(&player->cutscene_actor.collider);

    float playback_speed = 1.0f;
    bool repeat;
    struct animation_clip* next_clip = player_determine_animation(player, ground, &playback_speed, &repeat);

    if (player->cutscene_actor.state == ACTOR_STATE_IDLE) {
        player->cutscene_actor.animate_speed = playback_speed;
        if (next_clip != NULL && next_clip != player->cutscene_actor.animator.current_clip) {
            animator_run_clip(&player->cutscene_actor.animator, next_clip, 0.0f, repeat);
        }
    }

    if (cutscene_actor_update(&player->cutscene_actor) || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.d_up) {
        debug_collider_enable();
    } else if (pressed.d_down) {
        debug_collider_disable();
    }

    player_handle_movement(player, &input, ground);

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

    source->direction = castDirection;
    source->position = player->cutscene_actor.transform.position;
    source->position.y += 1.0f;
    source->flags.cast_state = input.btn.a ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;

    if (live_cast_has_pending_spell(&player->live_cast) && pressed.a) {
        spell_exec_start(&player->spell_exec, 4, live_cast_extract_active_spell(&player->live_cast), source);
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

    struct contact* contact = player->cutscene_actor.collider.active_contacts;

    while (contact) {
        struct collectable* collectable = collectable_get(contact->other_object);

        if (collectable) {
            collectable_collected(collectable);
        }

        contact = contact->next;
    }

    if (pressed.a) {
        player_handle_a_action(player);
    }

    player_check_inventory(player);
    live_cast_cleanup_unused_spells(&player->live_cast, &player->spell_exec);
}

float player_on_damage(void* data, struct damage_info* damage) {
    struct player* player = (struct player*)data;
    animator_run_clip(&player->cutscene_actor.animator, player->animations.take_damage, 0.0f, false);
    
    return damage->amount;
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

    spell_exec_init(&player->spell_exec);
    mana_pool_set_entity_id(&player->spell_exec.spell_sources.mana_pool, ENTITY_ID_PLAYER);
    live_cast_init(&player->live_cast);
    health_init(&player->health, ENTITY_ID_PLAYER, 100.0f);
    health_set_callback(&player->health, player_on_damage, player);

    for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
        struct spell_data_source* source = &player->player_spell_sources[i];

        source->flags.all = 0;
        source->reference_count = 1;
        source->target = ENTITY_ID_PLAYER;
    }

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

    player->animations.state = PLAYER_ANIMATION_GROUNDED;

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