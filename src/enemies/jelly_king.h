#ifndef __ENEMIES_JELLY_KING_H__
#define __ENEMIES_JELLY_KING_H__

#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "../render/tmesh.h"
#include "../scene/scene_definition.h"
#include "../render/renderable.h"
#include "../render/animator.h"
#include "jelly.h"
#include "../cutscene/cutscene_actor.h"

struct jelly_king_animations {
    struct animation_clip* idle;
    struct animation_clip* attack;
    struct animation_clip* attack_ranged;
    struct animation_clip* attack_aeo;
    struct animation_clip* attack_dash;
    struct animation_clip* die;
};

enum jelly_king_state {
    JELLY_KING_IDLE,
    JELLY_KING_BITE_AIM,
    JELLY_KING_ATTACK,
    JELLY_KING_ATTACK_RANGED,
    JELLY_KING_ATTACK_AIMING,
    JELLY_KING_ATTACK_AEO,
    JELLY_KING_ATTACK_DASH,
    JELLY_KING_FACE_PLAYER,
    JELLY_KING_DIE,
};

#define MAX_JELLY_MINIONS   5

struct jelly_king {
    cutscene_actor_t cutscene_actor;
    cutscene_actor_def_t actor_def;
    struct renderable renderable;
    struct health health;
    struct spatial_trigger vision;
    struct jelly_king_animations animations;
    struct Vector2 max_rotate;
    enum jelly_king_state state;
    contact_t last_target;

    union {
        struct {
            int number_left;
        } fire_minion;
        struct {
            int attack_timer;
        } idle;
        struct {
            float chase_timeout;
        } chase;
        struct {
            float aim_timer;
        } bite_aim;
        struct {
            bool did_bite;
        } bite;
        struct {
            bool did_dash;
        } dash;
    } state_data;

    entity_id minion[MAX_JELLY_MINIONS];

    uint8_t next_minion;
    uint8_t last_minion;
};

void jelly_king_init(struct jelly_king* jelly_king, struct jelly_king_definition* definition, entity_id id);
void jelly_king_destroy(struct jelly_king* jelly_king);
void jelly_king_common_init();
void jelly_king_common_destroy();

#endif