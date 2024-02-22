#include "player.h"

#include <libdragon.h>
#include "../resource/mesh_cache.h"
#include "../math/vector2.h"

#include "../render/render_scene.h"
#include "../time/time.h"

#define PLAYER_MAX_SPEED    3.0f

static struct Vector2 player_max_rotation;

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

    struct Vector2 direction;

    direction.x = input.stick_x * (1.0f / 80.0f);
    direction.y = -input.stick_y * (1.0f / 80.0f);

    float magSqrd = vector2MagSqr(&direction);

    if (magSqrd > 1.0f) {
        vector2Scale(&direction, 1.0f / sqrtf(magSqrd), &direction);
    }

    vector3AddScaled(&player->transform.position, &right, direction.x * PLAYER_MAX_SPEED * fixed_time_step, &player->transform.position);
    vector3AddScaled(&player->transform.position, &forward, direction.y * PLAYER_MAX_SPEED * fixed_time_step, &player->transform.position);

    if (magSqrd > 0.01f) {
        struct Vector2 directionUnit;

        vector2Normalize(&direction, &directionUnit);

        float tmp = directionUnit.x;
        directionUnit.x = directionUnit.y;
        directionUnit.y = tmp;

        vector2RotateTowards(&player->look_direction, &directionUnit, &player_max_rotation, &player->look_direction);
    }

    quatAxisComplex(&gUp, &player->look_direction, &player->transform.rotation);
}

void player_init(struct player* player, struct Transform* camera_transform) {
    transformInitIdentity(&player->transform);
    renderable_init(&player->renderable, &player->transform, "rom:/meshes/player/player.mesh");

    player->camera_transform = camera_transform;

    player->render_id = render_scene_add_renderable(&r_scene_3d, &player->renderable, 2.0f);
    player->update_id = update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD);

    player->look_direction = gRight2;

    vector2ComplexFromAngle(fixed_time_step * 3.14f, &player_max_rotation);
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);

    render_scene_remove(&r_scene_3d, player->render_id);
    update_remove(player->update_id);
    
}