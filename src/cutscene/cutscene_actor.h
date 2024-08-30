#ifndef __CUTSCENE_CUTSCENE_ACTOR_H__
#define __CUTSCENE_CUTSCENE_ACTOR_H__

#include "../math/vector3.h"
#include "../scene/scene_definition.h"
#include "../math/transform_mixed.h"
#include "../render/animator.h"
#include "../render/animation_clip.h"
#include "../render/armature.h"
#include <stdbool.h>

enum actor_state {
    ACTOR_STATE_IDLE,
    ACTOR_STATE_LOOKING,
    ACTOR_STATE_MOVING,
    ACTOR_STATE_LOOKING_MOVING,
    ACTOR_STATE_FINISHED,
};

struct cutscene_actor_animations {
    struct animation_clip* idle;
};

union cutscene_actor_id {
    struct {
        uint16_t npc_type;
        // used to tell apart two npcs of the same type
        uint16_t index;
    };
    uint32_t unique_id;
};

struct cutscene_actor {
    struct transform_mixed transform;
    struct animation_set* animation_set;
    struct cutscene_actor_animations animations;
    struct animator animator;
    struct armature* armature;
    struct Vector3 target;
    float move_speed;
    float animate_speed;
    enum actor_state state;
    union cutscene_actor_id id;
};

void cutscene_actor_init(struct cutscene_actor* actor, struct transform_mixed transform, enum npc_type npc_type, int index, struct armature* armature, char* animations_path);

void cutscene_actor_destroy(struct cutscene_actor* actor);
void cutscene_actor_reset();
struct cutscene_actor* cutscene_actor_find(enum npc_type npc_type, int index);

void cutscene_actor_look_at(struct cutscene_actor* actor, struct Vector3* at);
void cutscene_actor_move_to(struct cutscene_actor* actor, struct Vector3* at);
void cutscene_actor_idle(struct cutscene_actor* actor);

// return true if actor cutscene is active
bool cutscene_actor_update(struct cutscene_actor* actor);

#endif