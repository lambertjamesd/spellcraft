#ifndef __COLLIDER_SHAPE_H__
#define __COLLIDER_SHAPE_H__

#include "../math/vector2.h"
#include "../math/vector3.h"

enum collider_shape_type {
    COLLIDER_SHAPE_SPHERE,
    COLLIDER_SHAPE_CAPSULE,
    COLLIDER_SHAPE_BOX,
    COLLIDER_SHAPE_CYLINDER,
};

typedef enum collider_shape_type collider_shape_type_t;

struct swing_shape;

union collider_shape_data
{
    struct { float radius; } sphere;
    struct { float radius; float inner_half_height; } capsule;
    struct { struct Vector3 half_size; } box;
    struct { struct Vector3 size; } cone;
    struct { float radius; float half_height; } cylinder;
    struct { struct Vector2 range; float radius; float half_height; } sweep;
    struct { struct swing_shape* shape; } swing;
};

typedef union collider_shape_data collider_shape_data_t;

struct collider_shape {
    collider_shape_type_t type;
    vector3_t half_size;
    vector3_t center;
};

typedef struct collider_shape collider_shape_t;

#endif