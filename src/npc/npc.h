#ifndef __NPC_NPC_H__
#define __NPC_NPC_H__

#include "../scene/world_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../render/animator.h"

struct npc_animations {
    struct animation_clip* idle;
};

struct npc {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;

    struct animation_set* animation_set;
    struct npc_animations animations;
    struct animator animator;
};

void npc_init(struct npc* npc, struct npc_definition* definiton);
void npc_destroy(struct npc* npc);

#endif