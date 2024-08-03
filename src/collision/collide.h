#ifndef __COLLISION_COLLIDE_H__
#define __COLLISION_COLLIDE_H__

#include "mesh_collider.h"
#include "dynamic_object.h"
#include "epa.h"

void collide_object_to_mesh(struct dynamic_object* object, struct mesh_collider* mesh);
void collide_object_to_object(struct dynamic_object* a, struct dynamic_object* b);

void correct_velocity(struct dynamic_object* object, struct EpaResult* result, float ratio, float friction, float bounce);
void correct_overlap(struct dynamic_object* object, struct EpaResult* result, float ratio, float friction, float bounce);

void collide_add_contact(struct dynamic_object* object, struct EpaResult* result);

#endif