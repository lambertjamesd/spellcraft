#ifndef __COLLISION_DYNAMIC_OBJECT_H__
#define __COLLISION_DYNAMIC_OBJECT_H__

#include "../math/vector3.h"
#include "../math/box3d.h"
#include "../math/box2d.h"
#include "contact.h"
#include "gjk.h"
#include <stdint.h>

#define GRAVITY_CONSTANT    -9.8f

enum dynamic_object_flags {
    DYNAMIC_OBJECT_GRAVITY = (1 << 0),
};

typedef void (*bounding_box_calculator)(void* data, struct Vector2* rotation, struct Box3D* box);


union dynamic_object_type_data
{
    struct { float radius; } sphere;
    struct { struct Vector3 half_size; } box;
};

struct dynamic_object_type {
    MinkowsiSum minkowsi_sum;
    bounding_box_calculator bounding_box;
    union dynamic_object_type_data data;
};

struct dynamic_object {
    struct dynamic_object_type* type;
    struct Vector3* position;
    struct Vector2* rotation;
    struct Vector3 velocity;
    struct Box3D bounding_box;
    float time_scalar;
    uint16_t flags;
    struct contact* active_contacts;
};


void dynamic_object_init(
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    struct Vector3* position, 
    struct Vector2* rotation
);

void dynamic_object_update(struct dynamic_object* object);

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

void dynamic_object_box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

#endif