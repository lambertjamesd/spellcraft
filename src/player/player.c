#include "player.h"

#include <libdragon.h>
#include "../resource/mesh_cache.h"
#include "../resource/animation_cache.h"
#include "../math/vector2.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../objects/collectable.h"

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

struct spell_symbol projectile_spell_sybols[SPELL_MAX_COLS * SPELL_MAX_ROWS] = {
    {.reserved = 0, .type = SPELL_SYMBOL_PUSH},
    {.reserved = 0, .type = SPELL_SYMBOL_PROJECTILE},
    {.reserved = 0, .type = SPELL_SYMBOL_FIRE},
};

struct spell projectile_spell = {
    .symbols = projectile_spell_sybols,
    .cols = SPELL_MAX_COLS,
    .rows = SPELL_MAX_ROWS,
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

    if (speed < 0.000001f) {
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

void player_update(struct player* player) {
    struct Vector3 right;
    struct Vector3 forward;

    float playback_speed = 1.0f;
    struct animation_clip* next_clip = player_determine_animation(player, &playback_speed);

    if (next_clip != player->animator.current_clip) {
        animator_run_clip(&player->animator, next_clip, 0.0f, true);
    }

    animator_update(&player->animator, player->renderable.armature.pose, playback_speed * fixed_time_step);
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

    struct Vector3 directionWorld;
    vector3Scale(&right, &directionWorld, direction.x);
    vector3AddScaled(&directionWorld, &forward, direction.y, &directionWorld);

    float prev_y = player->collision.velocity.y;
    vector3Scale(&directionWorld, &player->collision.velocity, PLAYER_MAX_SPEED);
    player->collision.velocity.y = prev_y;

    if (magSqrd > 0.01f) {
        struct Vector2 directionUnit;

        directionUnit.x = directionWorld.x;
        directionUnit.y = directionWorld.z;

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

        if (player_cast_state(pressed, i) && player->inventory->spell_slots[i]) {
            spell_exec_start(&player->spell_exec, 0, player->inventory->spell_slots[i], source);
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
}

void player_init(struct player* player, struct Transform* camera_transform, struct inventory* inventory) {
    entity_id entity_id = entity_id_new();

    transformInitIdentity(&player->transform);
    renderable_init(&player->renderable, &player->transform, "rom:/meshes/characters/apprentice.mesh");

    player->camera_transform = camera_transform;
    player->inventory = inventory;

    player->transform.position.y = 1.0f;

    render_scene_add_renderable(&r_scene_3d, &player->renderable, 2.0f);
    update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD);

    player->look_direction = gRight2;

    vector2ComplexFromAngle(fixed_time_step * 7.0f, &player_max_rotation);

    dynamic_object_init(
        entity_id,
        &player->collision,
        &player_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_PLAYER,
        &player->transform.position,
        &player->look_direction
    );

    player->collision.center.y = player_collision.data.capsule.inner_half_height + player_collision.data.capsule.radius;

    collision_scene_add(&player->collision);

    spell_exec_init(&player->spell_exec);

    for (int i = 0; i < PLAYER_CAST_SOURCE_COUNT; i += 1) {
        struct spell_data_source* source = &player->player_spell_sources[i];

        source->flags.all = 0;
        source->reference_count = 1;
        source->target = entity_id;
    }

    player->animation_set = animation_cache_load("rom:/meshes/characters/apprentice.anim");
    player->animations.attack = animation_set_find_clip(player->animation_set, "attack1");
    player->animations.idle = animation_set_find_clip(player->animation_set, "idle");
    player->animations.run = animation_set_find_clip(player->animation_set, "run");

    animator_init(&player->animator, player->renderable.armature.bone_count);

    animator_run_clip(&player->animator, player->animations.idle, 0.0f, true);
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);
    spell_exec_destroy(&player->spell_exec);

    render_scene_remove(&r_scene_3d, &player->renderable);
    update_remove(player);
    collision_scene_remove(&player->collision);
    animation_cache_release(player->animation_set);
    animator_destroy(&player->animator);
}