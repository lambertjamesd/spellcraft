#ifndef __OBJECTS_DOOR_H__
#define __OBJECTS_DOOR_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../entity/interactable.h"
#include "../render/renderable.h"
#include "../render/animation_clip.h"
#include "../render/animator.h"
#include "../collision/dynamic_object.h"

struct door_animations {
    struct animation_clip* open;
    struct animation_clip* close;
};

struct door {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    room_id room_a;
    room_id room_b;
    boolean_variable unlocked;
    struct interactable interactable;
    struct animation_set* animation_set;
    struct door_animations animations;
    struct animator animator;
    struct dynamic_object collider;
    struct tmesh* lock_model;

    room_id next_room;
    room_id preview_room;
};

void door_init(struct door* door, struct door_definition* definition, entity_id id);
void door_destroy(struct door* door);
void door_common_init();
void door_common_destroy();

#endif