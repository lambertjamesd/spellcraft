#ifndef __COLLISION_COLLISION_SCENE_H__
#define __COLLISION_COLLISION_SCENE_H__

#include "dynamic_object.h"

typedef int collision_id;

void collision_scene_reset();
void collision_scene_add(struct dynamic_object* object);
void collision_scene_remove(struct dynamic_object* object);

void collision_scene_collide();

#endif