#ifndef __CUTSCENE_CUTSCENE_ACTOR_H__
#define __CUTSCENE_CUTSCENE_ACTOR_H__

#include "../math/vector3.h"
#include "../scene/scene_definition.h"
#include "../math/transform_mixed.h"
#include "../render/animator.h"
#include "../render/animation_clip.h"
#include <stdbool.h>

enum actor_state {
    ACTOR_STATE_INACTIVE,
    ACTOR_STATE_LOOKING,
    ACTOR_STATE_MOVING,
    ACTOR_STATE_FINISHED,
};

struct cutscene_actor {
    struct transform_mixed transform;
    struct Vector3* target;
    float speed;
    struct animator* animator;
    struct animation_set* animations;
    enum actor_state state;
};

void cutscene_actor_init(struct cutscene_actor* actor, enum npc_type npc_type, int index);
void cutscene_actor_destroy(struct cutscene_actor* actor);

void cutscene_actor_look_at(struct cutscene_actor* actor, struct Vector3* at);
void cutscene_actor_move_to(struct cutscene_actor* actor);

// return true if actor cutscene is active
bool cutscene_actor_update(struct cutscene_actor* actor);

#endif