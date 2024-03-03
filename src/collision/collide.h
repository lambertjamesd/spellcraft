#ifndef __COLLISION_COLLIDE_H__
#define __COLLISION_COLLIDE_H__

#include "mesh_collider.h"
#include "dynamic_object.h"

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh);
void collide_object_to_object(struct dynamic_object* a, struct dynamic_object* b);

#endif