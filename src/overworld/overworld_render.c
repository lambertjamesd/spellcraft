#include "overworld_render.h"

#include "../math/mathf.h"
#include "overworld_private.h"
#include "../render/defs.h"

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
            if (vector2DistSqr(&transformed_points[current_index], &transformed_points[next_index]) > 0.00001f) {
                result = next_index;
            }

            continue;
        }

        struct Vector2 edge_a;
        struct Vector2 edge_b;
        vector2Sub(&transformed_points[next_index], &transformed_points[current_index], &edge_a);
        vector2Sub(&transformed_points[result], &transformed_points[current_index], &edge_b);

        if (vector2Cross(&edge_a, &edge_b) > 0.00001f) {
            result = next_index;
        }
    }

    return result;
}

int overworld_create_top_view(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct Vector2* loop) {
    mat4x4 view_inv;

    if (!matrixInv(view_proj_matrix, view_inv)) {
        return 0;
    }

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
        
        transformed_points[i].x = (transformed_point.x * inv_w + camera_position->x - overworld->min.x) * overworld->inv_tile_size;
        transformed_points[i].y = (transformed_point.z * inv_w + camera_position->z - overworld->min.y) * overworld->inv_tile_size;

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

#define PREV_INDEX(state, current) ((current) == 0 ? state->loop_count - 1 : (current) - 1)
#define NEXT_INDEX(state, current) ((current) + 1 == state->loop_count ? 0 : (current) + 1)

float overworld_interpolate_x(struct Vector2* p0, struct Vector2* p1, float midy) {
    float t = (midy - p0->y) / (p1->y - p0->y);
    return (p1->x - p0->x) * t + p0->x;
}

struct overworld_tile_slice overworld_step(struct overworld* overworld, struct overworld_step_state* state) {
    float min_x = state->min_x;
    float max_x = state->max_x;

    float next_y = ceilf(state->current_y);

    if (next_y == state->current_y) {
        next_y += 1.0f;
    }

    bool result = true;

    for (int i = 0; i < state->loop_count; i += 1) {
        int prev_index = PREV_INDEX(state, state->left);

        struct Vector2* current_position = &state->loop[state->left];
        struct Vector2* prev_pos = &state->loop[prev_index];

        if (prev_pos->y < current_position->y) {
            result = false;
            break;
        }

        if (prev_pos->y < next_y) {
            min_x = minf(min_x, prev_pos->x);
            state->left = prev_index;
        } else {
            state->min_x = overworld_interpolate_x(current_position, prev_pos, next_y);
            min_x = minf(min_x, state->min_x);
            break;
        }
    }

    for (int i = 0; i < state->loop_count; i += 1) {
        int next_index = NEXT_INDEX(state, state->right);

        struct Vector2* current_position = &state->loop[state->right];
        struct Vector2* next_pos = &state->loop[next_index];

        if (next_pos->y < current_position->y) {
            result = false;
            break;
        }

        if (next_pos->y < next_y) {
            max_x = maxf(max_x, next_pos->x);
            state->right = next_index;
        } else {
            state->max_x = overworld_interpolate_x(current_position, next_pos, next_y);
            max_x = maxf(max_x, state->max_x);
            break;
        }
    }

    int y_int = (int)floorf(state->current_y);
    state->current_y = next_y;

    if (y_int < 0 || y_int >= overworld->tile_y) {
        return (struct overworld_tile_slice){
            .has_more = result && y_int < overworld->tile_y
        };
    }

    int min_int = (int)floorf(min_x);

    if (min_int < 0) {
        min_int = 0;
    }

    int max_int = (int)ceilf(max_x);

    if (max_int >= overworld->tile_x) {
        max_int = overworld->tile_x - 1;
    }

    return (struct overworld_tile_slice){
        .min_x = min_int,
        .max_x = max_int,
        .y = y_int,
        .has_more = result,
    };
}

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct frame_memory_pool* pool) {
    struct overworld_step_state state;
    state.loop_count = overworld_create_top_view(overworld, view_proj_matrix, camera_position, state.loop);
    state.left = 0;
    state.right = 0;
    state.current_y = state.loop[0].y;
    state.min_x = state.loop[0].x;
    state.max_x = state.loop[0].x;

    if (!state.loop_count) {
        return;
    }

    for (int i = 0; i < 4; i += 1) {
        struct overworld_tile_slice next = overworld_step(overworld, &state);

        for (int x = next.min_x; x < next.max_x; x += 1) {
            struct overworld_tile_render_block* block = &overworld->render_blocks[x & 0x3][next.y & 0x3];

            if (!block->render_block || block->x != x || block->y != next.y) {
                overworld->load_next.x = x;
                overworld->load_next.y = next.y;
                continue;
            }

            T3DMat4FP* tile_position = frame_malloc(pool, sizeof(T3DMat4FP));

            if (!tile_position) {
                return;
            }

            tile_position = UncachedAddr(tile_position);

            T3DMat4 mtx;
            t3d_mat4_identity(&mtx);
            t3d_mat4_translate(
                &mtx, 
                x * overworld->tile_size + overworld->min.x - camera_position->x,
                block->starting_y - camera_position->y,
                next.y * overworld->tile_size + overworld->min.y - camera_position->z
            );

            mtx.m[0][0] = 1.0f / SCENE_SCALE;
            mtx.m[1][1] = block->scale_y * (1.0f / SCENE_SCALE);
            mtx.m[2][2] = 1.0f / SCENE_SCALE; 

            t3d_mat4_to_fixed_3x4(tile_position, &mtx);

            t3d_matrix_push(tile_position);
            rspq_block_run(block->render_block);
            t3d_matrix_pop(1);
        }
        
        if (!next.has_more) {
            break;
        }
    }
}