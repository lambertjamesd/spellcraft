#ifndef __COLLISION_DYNAMIC_OBJECT_H__
#define __COLLISION_DYNAMIC_OBJECT_H__

#include "../entity/entity_id.h"
#include "../math/vector3.h"
#include "../math/box3d.h"
#include "../math/box2d.h"
#include "contact.h"
#include "gjk.h"
#include <stdint.h>
#include <stdbool.h>

#define GRAVITY_CONSTANT    -9.8f

enum collision_layers {
    COLLISION_LAYER_TANGIBLE = (1 << 0),
    COLLISION_LAYER_DAMAGE_PLAYER = (1 << 1),
    COLLISION_LAYER_DAMAGE_ENEMY = (1 << 2),
};

typedef void (*bounding_box_calculator)(void* data, struct Vector2* rotation, struct Box3D* box);

union dynamic_object_type_data
{
    struct { float radius; } sphere;
    struct { float radius; float inner_half_height; } capsule;
    struct { struct Vector3 half_size; } box;
    struct { struct Vector3 size; } cone;
    struct { float radius; float half_height; } cylinder;
};

struct dynamic_object_type {
    MinkowsiSum minkowsi_sum;
    bounding_box_calculator bounding_box;
    union dynamic_object_type_data data;
    float bounce;
    float friction;
};

struct dynamic_object {
    entity_id entity_id;
    struct dynamic_object_type* type;
    struct Vector3* position;
    struct Vector2* rotation;
    struct Vector2* pitch;
    struct Vector3 center;
    struct Vector3 velocity;
    struct Box3D bounding_box;
    float time_scalar;
    uint16_t has_gravity: 1;
    uint16_t is_trigger: 1;
    uint16_t is_fixed: 1;
    uint16_t collision_layers;
    struct contact* active_contacts;
};

void dynamic_object_init(
    entity_id entity_id,
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    uint16_t collision_layers,
    struct Vector3* position, 
    struct Vector2* rotation
);

void dynamic_object_update(struct dynamic_object* object);

struct contact* dynamic_object_nearest_contact(struct dynamic_object* object);
bool dynamic_object_is_touching(struct dynamic_object* object, entity_id id);

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_recalc_bb(struct dynamic_object* object);

void dynamic_object_box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_box_bouding_box(void* data, struct Vector2* rotation, struct Box3D* box);

void dynamic_object_cone_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_cone_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

void dynamic_object_sphere_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_sphere_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

void dynamic_object_capsule_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_capsule_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

void dynamic_object_cylinder_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_cylinder_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

#endif