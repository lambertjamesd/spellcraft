#include "player.h"

#include <libdragon.h>
#include "../resource/mesh_cache.h"
#include "../math/vector2.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../objects/collectable.h"

#define PLAYER_MAX_SPEED    3.0f

static struct Vector2 player_max_rotation;

static struct dynamic_object_type player_collision = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = dynamic_object_box_bouding_box,
    .data = {
        .box = {
            .half_size = {0.5f, 0.5f, 0.5f},
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

void player_update(struct player* player) {
    struct Vector3 right;
    struct Vector3 forward;
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

    float prev_y = player->collision.velocity.y;
    vector3Scale(&right, &player->collision.velocity, direction.x * PLAYER_MAX_SPEED);
    vector3AddScaled(&player->collision.velocity, &forward, direction.y * PLAYER_MAX_SPEED, &player->collision.velocity);
    player->collision.velocity.y = prev_y;

    if (magSqrd > 0.01f) {
        struct Vector2 directionUnit;

        vector2Normalize(&direction, &directionUnit);

        float tmp = directionUnit.x;
        directionUnit.x = directionUnit.y;
        directionUnit.y = tmp;

        vector2RotateTowards(&player->look_direction, &directionUnit, &player_max_rotation, &player->look_direction);
    }

    quatAxisComplex(&gUp, &player->look_direction, &player->transform.rotation);

    quatMultVector(&player->transform.rotation, &gForward, &player->player_spell_source.direction);
    player->player_spell_source.position = player->transform.position;
    player->player_spell_source.position.y += 0.5f;
    player->player_spell_source.flags.cast_state = input.btn.a ? SPELL_CAST_STATE_ACTIVE : SPELL_CAST_STATE_INACTIVE;

    if (pressed.a) {
        spell_exec_start(&player->spell_exec, 0, &player->inventory->custom_spells[0], &player->player_spell_source);
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

    vector2ComplexFromAngle(fixed_time_step * 3.14f, &player_max_rotation);

    dynamic_object_init(
        entity_id,
        &player->collision,
        &player_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_PLAYER,
        &player->transform.position,
        &player->look_direction
    );

    collision_scene_add(&player->collision);

    spell_exec_init(&player->spell_exec);

    player->player_spell_source.flags.all = 0;
    player->player_spell_source.reference_count = 1;
    player->player_spell_source.target = entity_id;
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);
    spell_exec_destroy(&player->spell_exec);

    render_scene_remove(&r_scene_3d, &player->renderable);
    update_remove(player);
    collision_scene_remove(&player->collision);
}