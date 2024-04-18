#ifndef __COLLISION_MESH_COLLIDER_H__
#define __COLLISION_MESH_COLLIDER_H__

#include <stdint.h>

#include "../math/vector3.h"
#include "../math/box3d.h"

struct mesh_triangle_indices {
    uint16_t indices[3];
};

struct mesh_index_block {
    uint16_t first_index;
    uint16_t last_index;
};

struct mesh_index {
    struct Vector3 min;
    struct Vector3 stride_inv;
    struct Vector3u8 block_count;

    struct mesh_index_block* blocks;
    uint16_t* index_indices;
};

struct mesh_collider {
    struct Vector3* vertices;
    struct mesh_triangle_indices* triangles;
    uint16_t triangle_count;

    struct mesh_index index;
};

struct mesh_triangle {
    struct Vector3* vertices;
    struct mesh_triangle_indices triangle;
};

void mesh_triangle_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

int mesh_index_lookup_triangle_indices(struct mesh_index* index, struct Box3D* box, uint16_t* indices, int max_index_count);

#endif