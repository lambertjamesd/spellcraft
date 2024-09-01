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
    ACTOR_STATE_IDLE = 0,
    ACTOR_STATE_LOOKING = (1 << 0),
    ACTOR_STATE_MOVING = (1 << 1),
    ACTOR_STATE_SPACE = (1 << 2),
    ACTOR_STATE_ACTIVE = (1 << 3),
};

struct cutscene_actor_animations {
    struct animation_clip* idle;
};

union cutscene_actor_id {
    struct {
        // used to tell apart two npcs of the same type
        uint16_t index;
        uint16_t npc_type;
    };
    uint32_t unique_id;
};

typedef union cutscene_actor_id cutscene_actor_id_t; 

struct cutscene_actor_def {
    float eye_level;
    float move_speed;
    float rotate_speed;
};

struct cutscene_actor {
    struct transform_mixed transform;
    struct animation_set* animation_set;
    struct cutscene_actor_animations animations;
    struct animator animator;
    struct armature* armature;
    struct Vector3 target;
    float animate_speed;
    enum actor_state state;
    cutscene_actor_id_t id;
    struct cutscene_actor_def* def;
};

void cutscene_actor_init(struct cutscene_actor* actor, struct cutscene_actor_def* def, struct transform_mixed transform, enum npc_type npc_type, int index, struct armature* armature, char* animations_path);

void cutscene_actor_destroy(struct cutscene_actor* actor);
void cutscene_actor_reset();
struct cutscene_actor* cutscene_actor_find(enum npc_type npc_type, int index);

void cutscene_actor_interact_with(struct cutscene_actor* actor, enum interaction_type interaction, struct Vector3* at);
void cutscene_actor_idle(struct cutscene_actor* actor);

bool cutscene_actor_is_moving(struct cutscene_actor* actor);

// return true if actor cutscene is active
bool cutscene_actor_update(struct cutscene_actor* actor);

#endif