#ifndef __COLLISION_CONTACT_H__
#define __COLLISION_CONTACT_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"

struct dynamic_object;

struct contact {
    struct contact* next;
    struct Vector3 point;
    struct Vector3 normal;
    entity_id other_object;
};

#endif