#ifndef __OBJECTS_DOOR_COMMON_H__
#define __OBJECTS_DOOR_COMMON_H__

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

typedef bool (*door_interact_blocker)(interactable_t* interactable, entity_id from);

struct door_base {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    room_id room_a;
    room_id room_b;
    struct interactable interactable;
    struct animation_set* animation_set;
    struct door_animations animations;
    struct animator animator;
    struct dynamic_object collider;
    struct tmesh* lock_model;

    room_id next_room;
    room_id preview_room;

    door_interact_blocker interact_blocker;
};

typedef struct door_base door_base_t;

struct door_base_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    room_id room_a;
    room_id room_b;
};

typedef struct door_base_definition door_base_definition_t;

void door_base_init(door_base_t* door, door_base_definition_t* definition, entity_id id, const char* mesh_filename);
void door_base_destroy(door_base_t* door);

void door_base_update(door_base_t* door);

void door_base_set_locked(door_base_t* door, bool value, door_interact_blocker interact_blocker);

#endif