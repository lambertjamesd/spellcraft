#ifndef __NPC_NPC_H__
#define __NPC_NPC_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../render/animator.h"
#include "../collision/dynamic_object.h"
#include "../entity/interactable.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_actor.h"

struct npc_information {
    char* mesh;
    char* animations;
    struct dynamic_object_type collider;
    float half_height;
    float eye_level;
};

struct npc {
    struct TransformSingleAxis transform;
    struct renderable renderable;

    struct cutscene_actor cutscene_actor;

    struct dynamic_object collider;

    struct interactable interactable;

    struct cutscene* talk_to_cutscene;
};

void npc_init(struct npc* npc, struct npc_definition* definiton);
void npc_destroy(struct npc* npc);

#endif