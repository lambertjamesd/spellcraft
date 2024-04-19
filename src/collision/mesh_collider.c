#include "mesh_collider.h"

#include <math.h>
#include "../math/minmax.h"

void mesh_triangle_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct mesh_triangle* triangle = (struct mesh_triangle*)data;

    int idx = 0;
    float distance = vector3Dot(&triangle->vertices[triangle->triangle.indices[0]], direction);

    float check = vector3Dot(&triangle->vertices[triangle->triangle.indices[1]], direction);

    if (check > distance) {
        idx = 1;
        distance = check;
    }

    check = vector3Dot(&triangle->vertices[triangle->triangle.indices[2]], direction);

    if (check > distance) {
        idx = 2;
    }

    *output = triangle->vertices[triangle->triangle.indices[idx]];
}

int mesh_index_merge_indices(struct mesh_index_block* block, uint16_t* index_indices, uint16_t* into, int max_index_count, int actual_index_count) {
    uint16_t* a = index_indices + block->first_index;
    uint16_t* b = into;

    uint16_t* max_a = index_indices + block->last_index;
    uint16_t* max_b = into + actual_index_count;

    uint16_t tmp[max_index_count];

    uint16_t* out = tmp;
    uint16_t* max_out = tmp + max_index_count;

    while (out < max_out && (a < max_a || b < max_b)) {
        if (b == max_b || (a < max_a && *a <= *b)) {
            if (*a == *b) {
                ++b;
            }

            *out++ = *a++;
        } else {
            *out++ = *b++;
        }
    }

    uint16_t* copy_from = tmp;

    while (copy_from < out) {
        *into++ = *copy_from++;
    }

    return out - tmp;
}

int mesh_index_lookup_triangle_indices(struct mesh_index* index, struct Box3D* box, uint16_t* indices, int max_index_count) {
    struct Vector3 offset;
    vector3Sub(&box->min, &index->min, &offset);
    struct Vector3 grid_index;
    vector3Multiply(&offset, &index->stride_inv, &grid_index);

    struct Vector3i32 min;
    min.x = floorf(grid_index.x);
    min.y = floorf(grid_index.y);
    min.z = floorf(grid_index.z);

    min.x = MAX(0, min.x);
    min.y = MAX(0, min.y);
    min.z = MAX(0, min.z);

    vector3Sub(&box->max, &index->min, &offset);
    vector3Multiply(&offset, &index->stride_inv, &grid_index);

    struct Vector3i32 max;
    max.x = ceilf(grid_index.x);
    max.y = ceilf(grid_index.y);
    max.z = ceilf(grid_index.z);

    max.x = MIN(index->block_count.x, max.x);
    max.y = MIN(index->block_count.x, max.y);
    max.z = MIN(index->block_count.x, max.z);

    int index_count = 0;

    struct mesh_index_block* start_block = &index->blocks[min.x + ((min.y + min.z * index->block_count.y) * index->block_count.x)];

    for (int z = min.z; z < max.z; z += 1) {
        struct mesh_index_block* start_row = start_block;

        for (int y = min.y; y < max.y; y += 1) {
            struct mesh_index_block* block = start_row;

            for (int x = min.x; x < max.x; x += 1) {
                index_count = mesh_index_merge_indices(block, index->index_indices, indices, max_index_count, index_count);

                if (index_count == max_index_count) {
                    return index_count;
                }

                block += 1;
            }

            start_row += index->block_count.x;
        }

        start_block += index->block_count.x * index->block_count.y;
    }

    return index_count;
}