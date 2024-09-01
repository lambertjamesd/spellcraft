#ifndef __SCENE_PLAYER_H__
#define __SCENE_PLAYER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector2.h"
#include "../render/render_batch.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../render/animation_clip.h"
#include "../render/animator.h"
#include "../cutscene/cutscene_actor.h"

#include "../spell/projectile.h"
#include "../spell/spell_exec.h"

#include "inventory.h"

#define PLAYER_CAST_SOURCE_COUNT    4

struct player_animations {
    struct animation_clip* idle;
    struct animation_clip* run;
    struct animation_clip* attack;
};

struct player_definition {
    struct Vector3 location;
    struct Vector2 rotation;
};

struct inventory_assets {
    struct tmesh* staffs[INV_STAFF_COUNT];
};

struct player {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct Transform* camera_transform;
    struct dynamic_object collision;

    struct spell_data_source player_spell_sources[PLAYER_CAST_SOURCE_COUNT];
    
    struct spell_exec spell_exec;

    struct player_animations animations;
    struct inventory_assets assets;

    struct cutscene_actor cutscene_actor;
};

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

#endif