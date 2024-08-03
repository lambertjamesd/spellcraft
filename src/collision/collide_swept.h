#ifndef __COLLISION_COLLIDE_SWEPT_H__
#define __COLLISION_COLLIDE_SWEPT_H__

#include "dynamic_object.h"
#include "mesh_collider.h"

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos);

#endif