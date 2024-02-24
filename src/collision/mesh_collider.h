#ifndef __COLLISION_MESH_COLLIDER_H__
#define __COLLISION_MESH_COLLIDER_H__

#include <stdint.h>

#include "../math/vector3.h"

struct mesh_triangle_indices {
    uint16_t indices[3];
};

struct mesh_collider {
    struct Vector3* vertices;
    struct mesh_triangle_indices* triangles;
    uint16_t triangle_count;
};

struct mesh_triangle {
    struct Vector3* vertices;
    struct mesh_triangle_indices triangle;
};

void mesh_triangle_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

#endif