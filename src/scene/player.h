#ifndef __SCENE_PLAYER_H__
#define __SCENE_PLAYER_H__

#include "../math/transform.h"
#include "../math/vector2.h"
#include "../render/mesh.h"
#include "../render/render_batch.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"

#include "../spell/projectile.h"
#include "../spell/spell_exec.h"

struct player {
    struct Transform transform;
    struct renderable renderable;
    struct Transform* camera_transform;
    int render_id;
    int update_id;
    struct Vector2 look_direction;
    struct dynamic_object collision;

    struct spell_data_source player_spell_source;
    
    struct spell_exec spell_exec;
};

void player_init(struct player* player, struct Transform* camera_transform);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

#endif