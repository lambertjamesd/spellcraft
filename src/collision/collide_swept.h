#ifndef __COLLISION_COLLIDE_SWEPT_H__
#define __COLLISION_COLLIDE_SWEPT_H__

#include "dynamic_object.h"
#include "mesh_collider.h"

struct swept_collide_result {
    struct Vector3 bounce_position;
    struct Vector3 bounce_velocity;
};

bool collide_object_to_mesh_swept(struct dynamic_object* object, struct mesh_collider* mesh, struct Vector3* prev_pos, struct swept_collide_result* result);

#endif