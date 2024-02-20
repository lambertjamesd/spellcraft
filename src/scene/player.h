#ifndef __SCENE_PLAYER_H__
#define __SCENE_PLAYER_H__

#include "../math/transform.h"
#include "../render/mesh.h"
#include "../render/render_batch.h"
#include "../render/renderable.h"

struct player {
    struct renderable renderable;
    int render_id;
};

void player_init(struct player* player);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

#endif