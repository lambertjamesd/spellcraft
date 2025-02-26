#include "overworld.h"

static int edge_deltas[] = {0x1, 0x2, 0x4};

int overworld_find_first_edge(struct Vector2 transformed_points[8], int current_index) {
    int result = current_index ^ 0x1;

    for (int i = 1; i < 3; i += 1) {
        int next_index = current_index ^ edge_deltas[i];

        if (transformed_points[next_index].x > transformed_points[result].x) {
            result = next_index;
        }
    }

    return result;
}

int overworld_find_next_edge(struct Vector2 transformed_points[8], int current_index, int prev_index) {
    int result = -1;

    for (int i = 0; i < 3; i += 1) {
        int next_index = current_index ^ edge_deltas[i];

        if (next_index == prev_index) {
            continue;
        }

        if (result == -1) {
            result = next_index;
            continue;
        }

        struct Vector2 edge_a;
        struct Vector2 edge_b;
        vector2Sub(&transformed_points[next_index], &transformed_points[current_index], &edge_a);
        vector2Sub(&transformed_points[result], &transformed_points[current_index], &edge_b);

        if (vector2Cross(&edge_a, &edge_b) > 0.0f) {
            result = next_index;
        }
    }

    return result;
}

int overworld_create_top_view(mat4x4 view_proj_matrix, struct Vector2* loop) {
    mat4x4 view_inv;

    matrixInv(view_proj_matrix, view_inv);

    struct Vector2 transformed_points[8];

    int current_index = 0;

    for (int i = 0; i < 8; i += 1) {
        struct Vector3 point = {
            (i & 0x1) ? 1.0f : -1.0f,
            (i & 0x2) ? 1.0f : -1.0f,
            (i & 0x4) ? 1.0f : -1.0f,
        };

        struct Vector4 transformed_point;
        matrixVec3Mul(view_inv, &point, &transformed_point);

        float inv_w = 1.0f / transformed_point.w;
        
        transformed_points[i].x = transformed_point.x * inv_w;
        transformed_points[i].y = transformed_point.y * inv_w;

        if (transformed_points[i].y < transformed_points[current_index].y) {
            current_index = i;
        }
    }

    int result = 0;

    loop[result++] = transformed_points[current_index];
    int prev_index = current_index;
    int start_index = current_index;
    current_index = overworld_find_first_edge(transformed_points, current_index);
    loop[result++] = transformed_points[current_index];

    while (result < 8) {
        int next_index = overworld_find_next_edge(transformed_points, current_index, prev_index);

        if (next_index == start_index) {
            break;
        }

        prev_index = current_index;
        current_index = next_index;

        loop[result++] = transformed_points[next_index];
    }

    return result;
}

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct frame_memory_pool* pool) {
    
}