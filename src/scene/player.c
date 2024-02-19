#include "player.h"

#include "../resource/mesh_cache.h"

void player_init(struct player* player) {
    transformInitIdentity(&player->transform);
    player->mesh = mesh_cache_load("rom:/meshes/player/player.mesh");
}

void player_render(struct player* player, struct render_batch* batch) {
    mat4x4* mtx = render_batch_get_transform(batch);
    if (!mtx) {
        return;
    }
    transformToMatrix(&player->transform, *mtx);
    render_batch_add_mesh(batch, player->mesh, mtx);
}

void player_destroy(struct player* player) {
    mesh_cache_release(player->mesh);
    player->mesh = NULL;
}