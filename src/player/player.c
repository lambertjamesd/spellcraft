#include "player.h"

#include <libdragon.h>
#include "../resource/animation_cache.h"
#include "../math/vector2.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../objects/collectable.h"
#include "../entity/interactable.h"
#include "../resource/tmesh_cache.h"

#define PLAYER_MAX_SPEED    4.2f

static struct Vector2 player_max_rotation;

static struct dynamic_object_type player_collision = {
    .minkowsi_sum = dynamic_object_capsule_minkowski_sum,
    .bounding_box = dynamic_object_capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.25f,
            .inner_half_height = 0.5f,
        }
    }
};

static struct dynamic_object_type player_visual_shape = {
    .minkowsi_sum = dynamic_object_cylinder_minkowski_sum,
    .bounding_box = dynamic_object_cylinder_bounding_box,
    .data = {
        .cylinder = {
            .half_height = 0.5f,
            .radius = 0.5f,
        }
    }
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

struct animation_clip* player_determine_animation(struct player* player, float* playback_speed) {
    struct Vector3 horizontal_velocity = player->collision.velocity;
    horizontal_velocity.y = 0.0f;
    float speed = sqrtf(vector3MagSqrd(&horizontal_velocity));

    if (speed < 0.1f) {
        *playback_speed = 1.0f;
        return player->animations.idle;
    }

    *playback_speed = speed * (1.0f / PLAYER_MAX_SPEED);
    return player->animations.run;
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

    struct Vector3 query_center = player->transform.position;
    struct Vector3 query_offset;
    quatMultVector(&player->transform.rotation, &gForward, &query_offset);
    vector3AddScaled(&query_center, &query_offset, 1.0f, &query_center);
    query_center.y += player_visual_shape.data.cylinder.half_height;
    bool did_intersect = false;
    collision_scene_query(&player_visual_shape, &query_center, COLLISION_LAYER_TANGIBLE, player_handle_interaction, &did_intersect);
}

void player_check_inventory(struct player* player) {
    struct staff_stats* staff = inventory_equipped_staff();
    player->renderable.attachments[0] = staff->item_type == ITEM_TYPE_NONE ? NULL : player->assets.staffs[staff->staff_index];
}

void player_update(struct player* player) {
    struct Vector3 right;
    struct Vector3 forward;

    float playback_speed = 1.0f;
    struct animation_clip* next_clip = player_determine_animation(player, &playback_speed);

    if (player->cutscene_actor.state == ACTOR_STATE_IDLE) {
        player->cutscene_actor.animate_speed = playback_speed;
        if (next_clip != player->cutscene_actor.animator.current_clip) {
            animator_run_clip(&player->cutscene_actor.animator, next_clip, 0.0f, true);
        }
    }

    if (cutscene_actor_update(&player->cutscene_actor)) {
        return;
    }

    player_get_move_basis(player->camera_transform, &forward, &right);

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    struct Vector2 direction;

    direction.x = input.stick_x * (1.0f / 80.0f);
    direction.y = -input.stick_y * (1.0f / 80.0f);

    float magSqrd = vector2MagSqr(&direction);

    if (magSqrd > 1.0f) {
        vector2Scale(&direction, 1.0f / sqrtf(magSqrd), &direction);
    }

    struct Vector3 directionScene;
    vector3Scale(&right, &directionScene, direction.x);
    vector3AddScaled(&directionScene, &forward, direction.y, &directionScene);

    float prev_y = player->collision.velocity.y;
    vector3Scale(&directionScene, &player->collision.velocity, PLAYER_MAX_SPEED);
    player->collision.velocity.y = prev_y;

    if (magSqrd > 0.01f) {
        struct Vector2 directionUnit;

        directionUnit.x = directionScene.x;
        directionUnit.y = directionScene.z;

        vector2Normalize(&directionUnit, &directionUnit);

        float tmp = directionUnit.x;
        directionUnit.x = directionUnit.y;
        directionUnit.y = tmp;

        vector2RotateTowards(&player->look_direction, &directionUnit, &player_max_rotation, &player->look_direction);
    }

    quatAxisComplex(&gUp, &player->look_direction, &player->transform.rotation);

    struct Vector3 castDirection;
    quatMultVector(&player->transform.rotation, &gForward, &castDirection);

    for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
        struct spell_data_source* source = &player->player_spell_sources[i];

        source->direction = castDirection;
        source->position = player->transform.position;
        source->position.y += 1.0f;
        source->flags.cast_state = player_cast_state(input.btn, i) ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;

        if (player_cast_state(pressed, i) && inventory_get_equipped_spell(i)) {
            spell_exec_start(&player->spell_exec, 0, inventory_get_equipped_spell(i), source);
        }
    }

    struct contact* contact = player->collision.active_contacts;

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
}

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform) {
    entity_id entity_id = entity_id_new();

    struct transform_mixed transform;
    transform_mixed_init(&transform, &player->transform);

    transformInitIdentity(&player->transform);
    renderable_init(&player->renderable, &player->transform, "rom:/meshes/characters/apprentice.tmesh");

    player->camera_transform = camera_transform;

    player->transform.position = definition->location;

    render_scene_add_renderable(&player->renderable, 2.0f);
    update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD);

    player->look_direction = definition->rotation;

    vector2ComplexFromAngle(fixed_time_step * 7.0f, &player_max_rotation);

    dynamic_object_init(
        entity_id,
        &player->collision,
        &player_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_PLAYER,
        &player->transform.position,
        &player->look_direction
    );

    player->collision.collision_group = COLLISION_GROUP_PLAYER;

    player->collision.center.y = player_collision.data.capsule.inner_half_height + player_collision.data.capsule.radius;

    collision_scene_add(&player->collision);

    spell_exec_init(&player->spell_exec);

    for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
        struct spell_data_source* source = &player->player_spell_sources[i];

        source->flags.all = 0;
        source->reference_count = 1;
        source->target = entity_id;
    }

    cutscene_actor_init(
        &player->cutscene_actor,
        transform,
        NPC_TYPE_PLAYER,
        0,
        &player->renderable.armature,
        "rom:/meshes/characters/apprentice.anim"
    );

    player->animations.attack = animation_set_find_clip(player->cutscene_actor.animation_set, "attack1");
    player->animations.idle = animation_set_find_clip(player->cutscene_actor.animation_set, "idle");
    player->animations.run = animation_set_find_clip(player->cutscene_actor.animation_set, "run");

    player->assets.staffs[0] = tmesh_cache_load("rom:/meshes/objects/staff_default.tmesh");
    player->assets.staffs[1] = NULL;
    player->assets.staffs[2] = NULL;
    player->assets.staffs[3] = NULL;
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);
    spell_exec_destroy(&player->spell_exec);

    render_scene_remove(&player->renderable);
    update_remove(player);
    collision_scene_remove(&player->collision);
    cutscene_actor_destroy(&player->cutscene_actor);
}