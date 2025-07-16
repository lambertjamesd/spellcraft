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
#include "../effects/drop_shadow.h"

#include "../spell/projectile.h"
#include "../spell/spell_exec.h"
#include "../spell/live_cast.h"
#include "../entity/health.h"

#include "inventory.h"

#define PLAYER_CAST_SOURCE_COUNT    5

enum player_state {
    PLAYER_GROUNDED,
    PLAYER_JUMPING,
    PLAYER_FALLING,
    PLAYER_SWIMMING,
    PLAYER_KNOCKBACK,
    PLAYER_GETTING_UP,
};

struct player_animations {
    struct animation_clip* idle;
    struct animation_clip* run;
    struct animation_clip* walk;
    struct animation_clip* dash;
    struct animation_clip* attack;
    struct animation_clip* attack_hold;
    struct animation_clip* air_dash;
    struct animation_clip* take_damage;

    struct animation_clip* tread_water;
    struct animation_clip* swim;

    struct animation_clip* jump;
    struct animation_clip* jump_peak;
    struct animation_clip* fall;
    struct animation_clip* land;

    struct animation_clip* knocked_back;
    struct animation_clip* knockback_fly;
    struct animation_clip* knockback_land;
};

struct player_definition {
    struct Vector3 location;
    struct Vector2 rotation;
};

struct inventory_assets {
    struct tmesh* staffs[INV_STAFF_COUNT];
};

struct player {
    struct cutscene_actor cutscene_actor;
    struct renderable renderable;
    struct Transform* camera_transform;
    struct health health;

    struct spell_data_source player_spell_sources[PLAYER_CAST_SOURCE_COUNT];
    
    struct spell_exec spell_exec;
    struct live_cast live_cast;

    struct player_animations animations;
    struct inventory_assets assets;
    
    struct drop_shadow drop_shadow;

    enum player_state state;
};

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

struct Vector3* player_get_position(struct player* player);

#endif