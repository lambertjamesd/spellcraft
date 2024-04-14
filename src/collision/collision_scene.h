#ifndef __COLLISION_COLLISION_SCENE_H__
#define __COLLISION_COLLISION_SCENE_H__

#include "dynamic_object.h"
#include "../collision/mesh_collider.h"
#include "contact.h"

typedef int collision_id;

void collision_scene_reset();
void collision_scene_add(struct dynamic_object* object);
void collision_scene_remove(struct dynamic_object* object);

struct dynamic_object* collision_scene_find_object(entity_id id);

void collision_scene_use_static_collision(struct mesh_collider* collider);
void collision_scene_remove_static_collision(struct mesh_collider* collider);

void collision_scene_collide();

struct contact* collision_scene_new_contact();

typedef void (*collision_scene_query_callback)(void* data, struct dynamic_object* overlaps);

void collision_scene_query(struct dynamic_object_type* shape, struct Vector3* center, int collision_layers, collision_scene_query_callback callback, void* callback_data);

#endif