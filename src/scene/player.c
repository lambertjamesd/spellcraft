#include "player.h"

#include "../resource/mesh_cache.h"

#include "../render/render_scene.h"

void player_init(struct player* player) {
    renderable_init(&player->renderable, "rom:/meshes/player/player.mesh");

    player->render_id = render_scene_add_renderable(&r_scene_3d, &player->renderable, 2.0f);
}

void player_destroy(struct player* player) {
    renderable_destroy(&player->renderable);

    render_scene_remove(&r_scene_3d, player->render_id);
}