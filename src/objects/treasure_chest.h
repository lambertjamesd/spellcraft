#ifndef __OBJECTS_TREASURE_CHEST_H__
#define __OBJECTS_TREASURE_CHEST_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../scene/scene_definition.h"
#include "../entity/interactable.h"
#include "../render/animation_clip.h"
#include "../render/animator.h"

struct treasure_animations {
    struct animation_clip* open;
};

struct treasure_chest {
    struct TransformSingleAxis transform;
    struct renderable_single_axis renderable;
    struct dynamic_object dynamic_object;
    struct interactable interactable;
    enum inventory_item_type item_type;
    struct animation_set* animation_set;
    struct treasure_animations animations;
    struct animator animator;
};

void treasure_chest_init(struct treasure_chest* treasure_chest, struct treasure_chest_definition* definition);
void treasure_chest_destroy(struct treasure_chest* treasure_chest);

#endif