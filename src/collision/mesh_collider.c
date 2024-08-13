#include "mesh_collider.h"

#include <math.h>
#include <stdio.h>
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
            if (b < max_b && *a == *b) {
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
    struct Vector3* max_point,
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
    next_point->x = dir_local->x > 0 ? ceilf(leading_point->x) : floorf(leading_point->x);
    next_point->y = dir_local->y > 0 ? ceilf(leading_point->y) : floorf(leading_point->y);
    next_point->z = dir_local->z > 0 ? ceilf(leading_point->z) : floorf(leading_point->z);

    vector3Add(leading_point, dir_local, max_point);

}

enum mesh_range_result {
    MESH_RANGE_RESULT_COLLIDE,
    MESH_RANGE_RESULT_SKIP,
    MESH_RANGE_RESULT_END,
};

enum mesh_range_result mesh_index_swept_determine_ranges(
    struct mesh_index* index,
    int axis,
    struct Vector3i32* min,
    struct Vector3i32* max,
    struct Vector3* dir_local,
    struct Vector3* leading_point,
    struct Vector3* next_point,
    struct Vector3* box_size
) {

    // TODO handle out of bounds

    enum mesh_range_result result = MESH_RANGE_RESULT_COLLIDE;

    int axis_index = (int)VECTOR3_AS_ARRAY(next_point)[axis];
    if (VECTOR3_AS_ARRAY(dir_local)[axis] > 0) {
        VECTOR3I_AS_ARRRAY(min)[axis] = axis_index - 1;
        VECTOR3I_AS_ARRRAY(max)[axis] = axis_index;

        if (VECTOR3I_AS_ARRRAY(min)[axis] >= VECTOR3U8_AS_ARRRAY(&index->block_count)[axis]) {
            return MESH_RANGE_RESULT_END;
        }
    } else {
        VECTOR3I_AS_ARRRAY(min)[axis] = axis_index;
        VECTOR3I_AS_ARRRAY(max)[axis] = axis_index + 1;

        if (VECTOR3I_AS_ARRRAY(max)[axis] <= 0) {
            return MESH_RANGE_RESULT_END;
        }
    }

    for (int curr_axis = 0; curr_axis < 3; curr_axis += 1) {
        if (curr_axis == axis) {
            continue;
        }

        if (VECTOR3_AS_ARRAY(dir_local)[curr_axis] > 0) {
            VECTOR3I_AS_ARRRAY(min)[curr_axis] = floorf(
                VECTOR3_AS_ARRAY(leading_point)[curr_axis] - VECTOR3_AS_ARRAY(box_size)[curr_axis]
            );
            VECTOR3I_AS_ARRRAY(max)[curr_axis] = ceilf(
                VECTOR3_AS_ARRAY(leading_point)[curr_axis]
            );
        } else {
            VECTOR3I_AS_ARRRAY(min)[curr_axis] = floorf(
                VECTOR3_AS_ARRAY(leading_point)[curr_axis]
            );
            VECTOR3I_AS_ARRRAY(max)[curr_axis] = ceilf(
                VECTOR3_AS_ARRAY(leading_point)[curr_axis] + VECTOR3_AS_ARRAY(box_size)[curr_axis]
            );
        }
    }

    min->x = MAX(min->x, 0);
    min->y = MAX(min->y, 0);
    min->z = MAX(min->z, 0);

    max->x = MIN(max->x, index->block_count.x);
    max->y = MIN(max->y, index->block_count.y);
    max->z = MIN(max->z, index->block_count.z);

    return result;
}

bool is_inf(float value) {
    return value == infinityf() || value == -infinityf();
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
        return true;
    }

    struct Vector3 dir_inv;
    struct Vector3 dir_local;
    struct Vector3 leading_point;
    struct Vector3 max_point;
    struct Vector3 next_point;

    mesh_index_swept_init(
        index,
        &start_position,
        move_amount,
        &dir_inv,
        &dir_local,
        &leading_point,
        &max_point,
        &next_point
    );

    int max_iterations = index->block_count.x + index->block_count.y + index->block_count.z;

    struct Vector3 box_size;
    vector3Sub(&end_position->max, &end_position->min, &box_size);
    // convert to index space
    vector3Multiply(&box_size, &index->stride_inv, &box_size);
    struct Vector3 next_offset;
    vector3Sub(&max_point, &leading_point, &next_offset);
    for (int iteration = 0; iteration < max_iterations && vector3Dot(&dir_local, &next_offset) > 0.0f; iteration += 1) {
        struct Vector3 offset_time;
        vector3Sub(&next_point, &leading_point, &next_offset);
        vector3Multiply(&next_offset, &dir_inv, &offset_time);

        int axis = 0;

        bool is_x_inf = is_inf(offset_time.x);
        bool is_y_inf = is_inf(offset_time.y);
        bool is_z_inf = is_inf(offset_time.z);

        if (!is_x_inf && 
            (offset_time.x < offset_time.y || is_y_inf) && 
            (offset_time.x < offset_time.z || is_z_inf)
        ) {
            axis = 0;
        } else if (!is_y_inf && 
            (offset_time.y < offset_time.z || is_z_inf)
        ) {
            axis = 1;
        } else {
            axis = 2;
        }

        VECTOR3_AS_ARRAY(&next_point)[axis] += VECTOR3_AS_ARRAY(&dir_local)[axis] > 0 ? 1 : -1;
        vector3AddScaled(&leading_point, &dir_local, VECTOR3_AS_ARRAY(&offset_time)[axis], &leading_point);
        
        enum mesh_range_result range_result = mesh_index_swept_determine_ranges(
            index, axis, &min, &max, &dir_local, &leading_point, &next_point, &box_size
        );

        if (range_result == MESH_RANGE_RESULT_END) {
            return false;
        }

        if (range_result == MESH_RANGE_RESULT_COLLIDE && mesh_index_traverse_index(index, &min, &max, callback, data, &indices[0], &index_count)) {
            return true;
        }

        vector3Sub(&max_point, &leading_point, &next_offset);
    }

    return false;
}

bool mesh_index_is_contained(struct mesh_index* index, struct Vector3* point) {
    struct Vector3 local_position;
    vector3Sub(point, &index->min, &local_position);
    vector3Multiply(&local_position, &index->stride_inv, &local_position);
    return local_position.x >= 0 && local_position.y >= 0 && local_position.z >= 0 &&
        local_position.x <= index->block_count.x && local_position.y <= index->block_count.y && local_position.z <= index->block_count.z;
}