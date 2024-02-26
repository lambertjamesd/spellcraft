#ifndef __COLLISION_CONTACT_H__
#define __COLLISION_CONTACT_H__

#include "../math/vector3.h"

struct dynamic_object;

struct contact {
    struct contact* next;
    struct Vector3 point;
    struct Vector3 normal;
    struct dynamic_object* other_object;
};

#endif