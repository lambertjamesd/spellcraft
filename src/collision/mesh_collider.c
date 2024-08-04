#include "mesh_collider.h"

#include <math.h>
#include "../math/minmax.h"

#define MAX_INDEX_SET_SIZE 64

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

bool mesh_index_merge_indices(
    struct mesh_index* index,
    struct mesh_index_block* block, 
    uint16_t* into, 
    int* actual_index_count,
    triangle_callback callback,
    void* data
) {
    uint16_t* a = index->index_indices + block->first_index;
    uint16_t* b = into;

    uint16_t* max_a = index->index_indices + block->last_index;
    uint16_t* max_b = into + *actual_index_count;

    uint16_t tmp[MAX_INDEX_SET_SIZE];

    uint16_t* out = tmp;
    uint16_t* max_out = tmp + MAX_INDEX_SET_SIZE;

    bool did_hit = false;

    while (a < max_a || b < max_b) {
        if (out == max_out) {
            // set overflowed, reset it to ensure every
            // triangle is checked even if it means
            // some will be checked more than once
            out = tmp;
            b = max_b;
        }

        if (b == max_b || (a < max_a && *a <= *b)) {
            if (*a == *b) {
                ++b;
            } else {
                // new index added
                did_hit = callback(index, data, *a) || did_hit;
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

    *actual_index_count = out - tmp;

    return did_hit;
}

bool mesh_index_traverse_index(
    struct mesh_index* index,
    struct Vector3i32* min,
    struct Vector3i32* max,
    triangle_callback callback,
    void* data,
    uint16_t* indices,
    int* index_count
) {
    bool did_hit = false;

    struct mesh_index_block* start_block = &index->blocks[min->x + ((min->y + min->z * index->block_count.y) * index->block_count.x)];

    for (int z = min->z; z < max->z; z += 1) {
        struct mesh_index_block* start_row = start_block;

        for (int y = min->y; y < max->y; y += 1) {
            struct mesh_index_block* block = start_row;

            for (int x = min->x; x < max->x; x += 1) {
                did_hit = mesh_index_merge_indices(
                    index,
                    block, 
                    indices, 
                    index_count,
                    callback,
                    data
                ) || did_hit;

                block += 1;
            }

            start_row += index->block_count.x;
        }

        start_block += index->block_count.x * index->block_count.y;
    }

    return did_hit;
}

void mesh_index_calculate_box(struct mesh_index* index, struct Box3D* box, struct Vector3i32* min, struct Vector3i32* max) {
    struct Vector3 offset;
    vector3Sub(&box->min, &index->min, &offset);
    struct Vector3 grid_index;
    vector3Multiply(&offset, &index->stride_inv, &grid_index);

    min->x = floorf(grid_index.x);
    min->y = floorf(grid_index.y);
    min->z = floorf(grid_index.z);

    min->x = MAX(0, min->x);
    min->y = MAX(0, min->y);
    min->z = MAX(0, min->z);

    vector3Sub(&box->max, &index->min, &offset);
    vector3Multiply(&offset, &index->stride_inv, &grid_index);

    max->x = ceilf(grid_index.x);
    max->y = ceilf(grid_index.y);
    max->z = ceilf(grid_index.z);

    max->x = MIN(index->block_count.x, max->x);
    max->y = MIN(index->block_count.y, max->y);
    max->z = MIN(index->block_count.z, max->z);
}

void mesh_index_lookup_triangle_indices(struct mesh_index* index, struct Box3D* box, triangle_callback callback, void* data) {
    struct Vector3i32 min;
    struct Vector3i32 max;

    mesh_index_calculate_box(index, box, &min, &max);
    uint16_t indices[MAX_INDEX_SET_SIZE];
    int index_count = 0;
    mesh_index_traverse_index(index, &min, &max, callback, data, &indices[0], &index_count);
}

float mesh_index_inv_offset(float input) {
    if (fabsf(input) < 0.000001f) {
        return infinityf();
    }

    return 1.0f / input;
}

void mesh_index_swept_init(
    struct mesh_index* index, 
    struct Box3D* start_position,
    struct Vector3* move_amount,
    struct Vector3* dir_inv,
    struct Vector3* dir_local,
    struct Vector3* leading_point,
    struct Vector3* next_point
) {
    // convert from world coordinates to index coordinates
    vector3Multiply(move_amount, &index->stride_inv, dir_local);
    dir_inv->x = mesh_index_inv_offset(dir_local->x);
    dir_inv->y = mesh_index_inv_offset(dir_local->y);
    dir_inv->z = mesh_index_inv_offset(dir_local->z);

    // determine the part of the bb that is in the direction of the sweep
    leading_point->x = dir_local->x > 0 ? start_position->max.x : start_position->min.x;
    leading_point->y = dir_local->y > 0 ? start_position->max.y : start_position->min.y;
    leading_point->z = dir_local->z > 0 ? start_position->max.z : start_position->min.z;

    // convert leading edge to local coordinates
    vector3Sub(leading_point, &index->min, leading_point);
    vector3Multiply(leading_point, &index->stride_inv, leading_point);

    // determine the next corner to check against
    next_point->x = dir_local->x > 0 ? floorf(leading_point->x) : ceilf(leading_point->x);
    next_point->y = dir_local->y > 0 ? floorf(leading_point->y) : ceilf(leading_point->y);
    next_point->z = dir_local->z > 0 ? floorf(leading_point->z) : ceilf(leading_point->z);

}

void mesh_index_swept_determine_ranges(
    int axis,
    struct Vector3i32* min,
    struct Vector3i32* max,
    struct Vector3* dir_local,
    struct Vector3* leading_point,
    struct Vector3* next_point,
    struct Vector3* box_size
) {
    int prev_axis = axis == 0 ? 2 : axis - 1;
    int next_axis = axis == 2 ? 0 : axis + 1;

    int axis_index = (int)VECTOR3_AS_ARRAY(next_point)[axis];
    if (VECTOR3_AS_ARRAY(dir_local)[axis] > 0) {
        VECTOR3I_AS_ARRRAY(min)[axis] = axis_index;
        VECTOR3I_AS_ARRRAY(max)[axis] = axis_index + 1;
    } else {
        VECTOR3I_AS_ARRRAY(min)[axis] = axis_index;
        VECTOR3I_AS_ARRRAY(max)[axis] = axis_index + 1;
    }

    if (VECTOR3_AS_ARRAY(dir_local)[prev_axis] > 0) {
        VECTOR3I_AS_ARRRAY(min)[prev_axis] = floorf(
            VECTOR3_AS_ARRAY(leading_point)[prev_axis] - VECTOR3_AS_ARRAY(box_size)[prev_axis]
        );
        VECTOR3I_AS_ARRRAY(min)[prev_axis] = ceilf(
            VECTOR3_AS_ARRAY(leading_point)[prev_axis]
        );
    } else {
        VECTOR3I_AS_ARRRAY(min)[prev_axis] = floorf(
            VECTOR3_AS_ARRAY(leading_point)[prev_axis]
        );
        VECTOR3I_AS_ARRRAY(min)[prev_axis] = ceilf(
            VECTOR3_AS_ARRAY(leading_point)[prev_axis] + VECTOR3_AS_ARRAY(box_size)[prev_axis]
        );
    }

    if (VECTOR3_AS_ARRAY(dir_local)[next_axis] > 0) {
        VECTOR3I_AS_ARRRAY(min)[next_axis] = floorf(
            VECTOR3_AS_ARRAY(leading_point)[next_axis] - VECTOR3_AS_ARRAY(box_size)[next_axis]
        );
        VECTOR3I_AS_ARRRAY(min)[next_axis] = ceilf(
            VECTOR3_AS_ARRAY(leading_point)[next_axis]
        );
    } else {
        VECTOR3I_AS_ARRRAY(min)[next_axis] = floorf(
            VECTOR3_AS_ARRAY(leading_point)[next_axis]
        );
        VECTOR3I_AS_ARRRAY(min)[next_axis] = ceilf(
            VECTOR3_AS_ARRAY(leading_point)[next_axis] + VECTOR3_AS_ARRAY(box_size)[next_axis]
        );
    }
}

bool mesh_index_swept_lookup(struct mesh_index* index, struct Box3D* end_position, struct Vector3* move_amount, triangle_callback callback, void* data) {
    struct Box3D start_position;
    vector3Sub(&end_position->min, move_amount, &start_position.min);
    vector3Sub(&end_position->max, move_amount, &start_position.max);

    struct Vector3i32 min;
    struct Vector3i32 max;
    mesh_index_calculate_box(index, &start_position, &min, &max);
    uint16_t indices[MAX_INDEX_SET_SIZE];
    int index_count = 0;
    if (mesh_index_traverse_index(index, &min, &max, callback, data, &indices[0], &index_count)) {
        return false;
    }

    struct Vector3 dir_inv;
    struct Vector3 dir_local;
    struct Vector3 leading_point;
    struct Vector3 next_point;

    mesh_index_swept_init(
        index,
        &start_position,
        move_amount,
        &dir_inv,
        &dir_local,
        &leading_point,
        &next_point
    );

    struct Vector3 box_size;
    vector3Sub(&end_position->max, &end_position->min, &box_size);
    struct Vector3 next_offset;
    vector3Sub(&next_point, &leading_point, &next_offset);
    while (vector3Dot(&dir_local, &next_offset) < 0.0f) {
        struct Vector3 offset_time;
        vector3Multiply(&next_offset, &dir_inv, &offset_time);

        int axis;

        if (offset_time.x < offset_time.y && offset_time.x < offset_time.z) {
            axis = 0;
        } else if (offset_time.y < offset_time.z) {
            axis = 1;
        } else {
            axis = 2;
        }

        VECTOR3_AS_ARRAY(&next_point)[axis] += VECTOR3_AS_ARRAY(&dir_local)[axis] > 0 ? 1 : -1;
        vector3AddScaled(&leading_point, &dir_local, VECTOR3_AS_ARRAY(&offset_time)[axis], &leading_point);

        // TODO handle out of bounds
        
        mesh_index_swept_determine_ranges(
            axis, &min, &max, &dir_local, &leading_point, &next_point, &box_size
        );

        if (mesh_index_traverse_index(index, &min, &max, callback, data, &indices[0], &index_count)) {
            return true;
        }

        vector3Sub(&next_point, &leading_point, &next_offset);
    }

    return false;
}