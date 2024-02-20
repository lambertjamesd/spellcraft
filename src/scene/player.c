#include "player.h"

#include "../resource/mesh_cache.h"

#include "../render/render_scene.h"
#include "../time/time.h"

void player_update(struct player* player) {
    player->transform.position.x += fixed_time_step * 0.5f;
}

void player_init(struct player* player) {
    transformInitIdentity(&player->transform);
    renderable_init(&player->renderable, &player->transform, "rom:/meshes/player/player.mesh");

    player->render_id = render_scene_add_renderable(&r_scene_3d, &player->renderable, 2.0f);
    player->update_id = update_add(player, (update_callback)player_update, 0, UPDATE_LAYER_WORLD);
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);

    render_scene_remove(&r_scene_3d, player->render_id);
    update_remove(player->update_id);
}