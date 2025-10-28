#ifndef __SCENE_PLAYER_H__
#define __SCENE_PLAYER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector2.h"
#include "../render/render_batch.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../render/animation_clip.h"
#include "../render/animator.h"
#include "../cutscene/cutscene_actor.h"
#include "../effects/drop_shadow.h"

#include "../spell/projectile.h"
#include "../spell/spell_exec.h"
#include "../spell/live_cast.h"
#include "../entity/health.h"
#include "../entity/interactable.h"

#include "inventory.h"
#include "grab_checker.h"

#define PLAYER_CAST_SOURCE_COUNT    5
#define CLIMB_UP_COUNT              3

enum player_state {
    PLAYER_GROUNDED,
    PLAYER_SLIDING,
    PLAYER_JUMPING,
    PLAYER_FALLING,
    PLAYER_SWIMMING,
    PLAYER_KNOCKBACK,
    PLAYER_GETTING_UP,
    PLAYER_CLIMBING_UP,
    PLAYER_CARRY,
};

enum player_animation {
    PLAYER_ANIMATION_IDLE,
    PLAYER_ANIMATION_RUN,
    PLAYER_ANIMATION_WALK,
    PLAYER_ANIMATION_DASH,
    PLAYER_ANIMATION_ATTACK,
    PLAYER_ANIMATION_ATTACK_HOLD,
    PLAYER_ANIMATION_AIR_DASH,
    PLAYER_ANIMATION_TAKE_DAMAGE,
    
    PLAYER_ANIMATION_TREAD_WATER,
    PLAYER_ANIMATION_SWIM,
    
    PLAYER_ANIMATION_JUMP,
    PLAYER_ANIMATION_JUMP_PEAK,
    PLAYER_ANIMATION_FALL,
    PLAYER_ANIMATION_LAND,

    PLAYER_ANIMATION_KNOCKED_BACK,
    PLAYER_ANIMATION_KNOCKBACK_FLY,
    PLAYER_ANIMATION_KNOCKBACK_LAND,

    PLAYER_ANIMATION_SWING_ATTACK,
    PLAYER_ANIMATION_SPIN_ATTACK,
    PLAYER_ANIMATION_CAST_UP,
    
    PLAYER_ANIMATION_CLIMB_UP_0,
    PLAYER_ANIMATION_CLIMB_UP_1,
    PLAYER_ANIMATION_CLIMB_UP_2,
    
    PLAYER_ANIMATION_CARRY_PICKUP,
    PLAYER_ANIMATION_CARRY_IDLE,
    PLAYER_ANIMATION_CARRY_RUN,
    PLAYER_ANIMATION_CARRY_WALK,
    PLAYER_ANIMATION_CARRY_DROP,

    PLAYER_ANIMATION_COUNT,
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

    struct animation_clip* swing_attack;
    struct animation_clip* spin_attack;
    struct animation_clip* cast_up;
    
    struct animation_clip* climb_up[CLIMB_UP_COUNT];

    struct animation_clip* carry_pickup;
    struct animation_clip* carry_idle;
    struct animation_clip* carry_run;
    struct animation_clip* carry_walk;
    struct animation_clip* carry_drop;
};

struct player_definition {
    struct Vector3 location;
    struct Vector2 rotation;
};

struct inventory_assets {
    struct tmesh* staffs[INV_STAFF_COUNT];
};

union state_data {
    struct {
        float timer;
        float y_velocity;
        struct Vector3 start_pos;
        struct Vector2 target_rotation;
        uint8_t climb_up_index;
    } climbing_up;
    struct {
        entity_id carrying;
        float carry_offset;
        bool should_carry;
    } carrying;
};

typedef union state_data state_data_t;

struct player {
    struct cutscene_actor cutscene_actor;
    struct renderable renderable;
    struct Transform* camera_transform;
    struct health health;

    struct spell_data_source player_spell_sources[PLAYER_CAST_SOURCE_COUNT];
    
    struct spell_exec spell_exec;
    struct live_cast live_cast;
    grab_checker_t grab_checker;

    animation_clip_t* animations[PLAYER_ANIMATION_COUNT];
    struct animation_clip* last_spell_animation;
    struct inventory_assets assets;
    
    struct drop_shadow drop_shadow;

    struct Vector3 last_good_footing;
    float slide_timer;
    float coyote_time;

    enum player_state state;
    union state_data state_data;

    struct spatial_trigger z_target_trigger;
    struct TransformSingleAxis z_target_transform;
    struct renderable z_target_visual;

    entity_id z_target;
    interact_type_t last_interaction_type;
};

typedef struct player player_t;

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

struct Vector3* player_get_position(struct player* player);

#endif