#include "overworld_render.h"

#include <stdlib.h>
#include "../math/mathf.h"
#include "overworld_private.h"
#include "../render/defs.h"
#include "../math/vector2s16.h"
#include "../profile/profile.h"

static int edge_deltas[] = {0x1, 0x2, 0x4};

#if ENABLE_LOD_RENDER_DEBUG
int lod_render_mode = LOD_RENDER_MODE_DEFAULT;
#endif

int overworld_find_next_edge(struct Vector2 transformed_points[8], int current_index, int prev_index) {
    int result = -1;

    for (int i = 0; i < 3; i += 1) {
        int next_index = current_index ^ edge_deltas[i];

        if (next_index == prev_index) {
            continue;
        }

        if (vector2DistSqr(&transformed_points[current_index], &transformed_points[next_index]) < 0.00001f) {
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

        float inv_w = 1.0f / (transformed_point.w * WORLD_SCALE);
        
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
    current_index = overworld_find_next_edge(transformed_points, current_index, current_index);
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

    if (max_int > overworld->tile_x) {
        max_int = overworld->tile_x;
    }

    return (struct overworld_tile_slice){
        .min_x = min_int,
        .max_x = max_int,
        .y = y_int,
        .has_more = result,
    };
}

struct overworld_lod1_sort_entry {
    uint32_t priority;
    struct tmesh* mesh;
};

int overworld_entry_sort(const void* a, const void* b) {
    const struct overworld_lod1_sort_entry* a_entry = (const struct overworld_lod1_sort_entry*)a;
    const struct overworld_lod1_sort_entry* b_entry = (const struct overworld_lod1_sort_entry*)b;
    return (int)(b_entry->priority - a_entry->priority);
}

int overworld_lod_1_direction_index(int dx, int dy) {
    if (abs(dx) > abs(dy)) {
        return dx > 0 ? 1 : 0;
    }

    return dy > 0 ? 3 : 2;
}

#define UNIT_SCALE  2000

void overworld_create_2d_clipping_planes(quaternion_t* camera_rotation, float tan_fov, float aspect_ratio, vector2s16_t* output) {
    vector2_t normal;

    normal.x = tan_fov * aspect_ratio;
    normal.y = 1.0f;

    vector2Rotate90(&normal, &normal);
    vector2Normalize(&normal, &normal);

    vector3_t camera_forward;
    quatMultVector(camera_rotation, &gBackward, &camera_forward);
    vector2_t camera_rotation_2d;
    vector2LookDir(&camera_rotation_2d, &camera_forward);

    for (int i = 0; i < 2; i += 1) {
        vector2_t rotated;
        vector2ComplexMul(&normal, &camera_rotation_2d, &rotated);
        output[i].x = (int16_t)(UNIT_SCALE * rotated.x);
        output[i].y = (int16_t)(UNIT_SCALE * rotated.y);
    
        normal.x = -normal.x;
    }
}

#define CULL_TOLERANCE          1200000
#define SKYBOX_RENDER_OFFSET    100

#define LEVEL2_MIN_DISTANCE     500

void overworld_render_lod_1_entries(struct overworld_lod1* lod1, int camera_x, int camera_z, T3DMat4FP* mtx, T3DMat4FP* skybox_mtx, vector2s16_t* clipping_planes) {
    struct overworld_lod1_sort_entry order[lod1->entry_count];

    struct overworld_lod1_entry* end = lod1->entries + lod1->entry_count;
    struct overworld_lod1_sort_entry* entry = order;
    
    for (struct overworld_lod1_entry* curr = lod1->entries; curr < end; curr += 1) {
        vector2s16_t delta = {
            .x = curr->x - camera_x,
            .y = curr->z - camera_z
        };

        bool should_cull = false;

        for (int plane = 0; curr->priority < SKYBOX_RENDER_OFFSET && plane < 2; plane += 1) {
            if (vector2s16Dot(&delta, &clipping_planes[plane]) > CULL_TOLERANCE * curr->lod_scale) {
                should_cull = true;
                break;
            }
        }

        if (should_cull) {
            curr += curr->child_count;
            continue;
        }

        bool should_skip_children = false;

        int distance = (int)delta.x * (int)delta.x + (int)delta.y * (int)delta.y;

        if (!USE_LESS_MEMORY && curr->lod_scale > 1 && distance < LEVEL2_MIN_DISTANCE * LEVEL2_MIN_DISTANCE * curr->lod_scale * curr->lod_scale) {
            continue;
        } else {
            should_skip_children = true;
        }

        entry->priority = (distance >> 2) + ((uint32_t)curr->priority << 24);
        
        entry->mesh = &curr->meshes[overworld_lod_1_direction_index(delta.x, delta.y)];
        entry += 1;

        if (should_skip_children) {
            curr += curr->child_count;
        }
    }

    int final_count = entry - order;

    qsort(order, final_count, sizeof(struct overworld_lod1_sort_entry), overworld_entry_sort);

    struct material* mat = NULL;

    T3DMat4FP* curr_mtx = NULL;
    
#if ENABLE_LOD_RENDER_DEBUG
    if (lod_render_mode == LOD_RENDER_MODE_DETAILED) {
        return;
    } else if (lod_render_mode >= 0 && lod_render_mode < final_count) {
        final_count = lod_render_mode;
    }
#endif

    for (int i = 0; i < final_count; i += 1) {
        struct tmesh* mesh = order[i].mesh;

        if (mat != mesh->material) {
            material_apply(mesh->material);
            mat = mesh->material;
        }

        T3DMat4FP* use_mtx = (order[i].priority >> 24) >= SKYBOX_RENDER_OFFSET ? skybox_mtx : mtx;

        if (use_mtx != curr_mtx) {
            if (curr_mtx) {
                t3d_matrix_pop(1);
            }
            
            t3d_matrix_push(use_mtx);
            curr_mtx = use_mtx;
        }

        rspq_block_run(mesh->block);
    }

    if (curr_mtx) {
        t3d_matrix_pop(1);
    }
}

#define CENTER_SCALE    0.5f

void overworld_render_lod_1(struct overworld* overworld, struct Camera* camera, T3DViewport* prev_viewport, struct frame_memory_pool* pool) {
    T3DViewport* new_viewport = frame_malloc(pool, sizeof(T3DViewport));
    *new_viewport = t3d_viewport_create();
    
    float lod_scale = 1.0f / overworld->tile_x;
    float aspect_ratio = (float)new_viewport->size[0] / (float)new_viewport->size[1];

    camera_t lod_1_camera = *camera;
    lod_1_camera.near = (camera->far * 0.25f) * lod_scale;
    lod_1_camera.far = overworld->tile_size * 1.4f;
    
    float tan_fov = tanf(camera->fov * DEG_TO_RAD(0.5f));
    camera_apply(&lod_1_camera, new_viewport, NULL, NULL);

    t3d_viewport_set_w_normalize(new_viewport, camera->near * WORLD_SCALE, camera->far * WORLD_SCALE);

    t3d_viewport_attach(new_viewport);

    T3DMat4 mtx;
    t3d_mat4_identity(&mtx);
    t3d_mat4_translate(
        &mtx, 
        -camera->transform.position.x * lod_scale * WORLD_SCALE,
        -camera->transform.position.y * lod_scale * WORLD_SCALE,
        -camera->transform.position.z * lod_scale * WORLD_SCALE
    );
    mtx.m[0][0] = STATIC_WORLD_SCALE;
    mtx.m[1][1] = STATIC_WORLD_SCALE;
    mtx.m[2][2] = STATIC_WORLD_SCALE;

    int camera_x = -(int)(mtx.m[3][0] * CENTER_SCALE);
    int camera_z = -(int)(mtx.m[3][2] * CENTER_SCALE);

    rdpq_sync_pipe();
    rdpq_mode_zbuf(false, false);

    T3DMat4FP* mtx_fp = UncachedAddr(frame_malloc(pool, sizeof(T3DMat4FP)));
    t3d_mat4_to_fixed_3x4(mtx_fp, &mtx);
    
    T3DMat4FP* skybox_mtx = UncachedAddr(frame_malloc(pool, sizeof(T3DMat4FP)));
    mtx.m[3][0] = 0.0f;
    mtx.m[3][1] = 0.0f;
    mtx.m[3][2] = 0.0f;
    t3d_mat4_to_fixed_3x4(skybox_mtx, &mtx);

    vector2s16_t clipping_planes[2];
    overworld_create_2d_clipping_planes(&camera->transform.rotation, tan_fov, aspect_ratio, clipping_planes);
    overworld_render_lod_1_entries(&overworld->lod1, camera_x, camera_z, mtx_fp, skybox_mtx, clipping_planes);

    rdpq_sync_pipe();
    rdpq_mode_zbuf(true, true);
} 

struct overworld_tile_render_info {
    T3DMat4FP* transform;
    overworld_tile_layer_t* layer;
    struct overworld_tile_render_block* particles_block;
};

typedef struct overworld_tile_render_info overworld_tile_render_info_t;


overworld_tile_render_info_t* overworld_tile_enumerate_tiles(struct overworld* overworld, struct Camera* camera, struct frame_memory_pool* pool, int x, int z, overworld_tile_render_info_t* curr) {
    struct overworld_tile_render_block* block = &overworld->render_blocks[x & 0x3][z & 0x3];
    struct Vector3* camera_position = &camera->transform.position;
    
    if (!block->layers || block->x != x || block->z != z) {
        overworld->load_next.x = x;
        overworld->load_next.y = z;
        return curr;
    }
    
    int min_y = (int)floorf((camera_position->y - block->starting_y - camera->far) * overworld->inv_tile_size);
    int max_y = (int)ceilf((camera_position->y - block->starting_y + camera->far) * overworld->inv_tile_size);

    if (min_y < 0) {
        min_y = 0;
    }

    if (max_y > block->y_height) {
        max_y = block->y_height;
    }
    
    T3DMat4 mtx;
    t3d_mat4_identity(&mtx);

    t3d_mat4_translate(
        &mtx, 
        (x * overworld->tile_size + overworld->min.x - camera_position->x) * WORLD_SCALE,
        (min_y * overworld->tile_size + block->starting_y - camera_position->y) * WORLD_SCALE,
        (z * overworld->tile_size + overworld->min.y - camera_position->z) * WORLD_SCALE
    );

    mtx.m[0][0] = STATIC_WORLD_SCALE;
    mtx.m[1][1] = STATIC_WORLD_SCALE;
    mtx.m[2][2] = STATIC_WORLD_SCALE;

    for (int y = min_y; y < max_y; y += 1, ++curr) {
        T3DMat4FP* tile_position = frame_malloc(pool, sizeof(T3DMat4FP));
    
        if (!tile_position) {
            return curr;
        }
    
        tile_position = UncachedAddr(tile_position);
    
        t3d_mat4_to_fixed_3x4(tile_position, &mtx);

        *curr = (overworld_tile_render_info_t) {
            .transform = tile_position,
            .layer = &block->layers[y],
            .particles_block = y == min_y ? block : NULL,
        };
        
        mtx.m[3][1] += overworld->tile_size * WORLD_SCALE;
    }

    return curr;
}

void overworld_render_low_priority(overworld_tile_render_info_t* curr) {
    t3d_matrix_push(curr->transform);
    
    for (int i = 0; i < curr->layer->pre_scrolling_mesh_count; i += 1) {
        material_apply(curr->layer->scrolling_meshes[i].material);
        rdpq_mode_zbuf(false, false);
        rspq_block_run(curr->layer->scrolling_meshes[i].block);
    }
    
    t3d_matrix_pop(1);
}

void overworld_render_tile(overworld_tile_render_info_t* curr) {
    t3d_matrix_push(curr->transform);

    rspq_block_run(curr->layer->render_block);

    for (int i = curr->layer->pre_scrolling_mesh_count; i < curr->layer->scrolling_mesh_count; i += 1) {
        material_apply(curr->layer->scrolling_meshes[i].material);
        rspq_block_run(curr->layer->scrolling_meshes[i].block);
    }

    t3d_matrix_pop(1);
}

void overworld_render_particles(overworld_tile_render_info_t* curr, vector3_t* camera_pos, struct frame_memory_pool* pool) {
    struct overworld_tile_render_block* block = curr->particles_block;

    if (block && block->tile->static_particle_count) {
        static_particles_start();
        static_particles_render_instances(block->tile->static_particles, block->tile->static_particle_count, pool, camera_pos);
        static_particles_end();
    }
}

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    struct overworld_step_state state;
    struct Vector3* camera_position = &camera->transform.position;
    state.loop_count = overworld_create_top_view(overworld, view_proj_matrix, camera_position, state.loop);
    state.left = 0;
    state.right = 0;
    state.current_y = state.loop[0].y;
    state.min_x = state.loop[0].x;
    state.max_x = state.loop[0].x;

    SC_PROFILE_START(render);
    overworld_render_lod_1(overworld, camera, viewport, pool);
    SC_PROFILE_END(render, overworld_render_lod_1);

    t3d_viewport_attach(viewport);

#if ENABLE_LOD_RENDER_DEBUG
    if (lod_render_mode == LOD_RENDER_MODE_LOD3 || lod_render_mode >= 0) {
        return;
    }
#endif

    if (!state.loop_count) {
        return;
    }

    overworld_tile_render_info_t tiles[8];
    overworld_tile_render_info_t* block = tiles;
    
    SC_PROFILE_START(render);

    for (int i = 0; i < 4; i += 1) {
        struct overworld_tile_slice next = overworld_step(overworld, &state);

        for (int x = next.min_x; x < next.max_x; x += 1) {
            block = overworld_tile_enumerate_tiles(overworld, camera, pool, x, next.y, block);
        }
        
        if (!next.has_more) {
            break;
        }
    }
    
    SC_PROFILE_END(render, overworld_tile_enumerate_tiles);
    
    SC_PROFILE_START(render);
    for (overworld_tile_render_info_t* curr = tiles; curr < block; ++curr) {
        overworld_render_low_priority(curr);
    }
    SC_PROFILE_END(render, overworld_render_low_priority);

    rdpq_sync_pipe();
    rdpq_mode_zbuf(true, true);
    
    SC_PROFILE_START(render);
    for (overworld_tile_render_info_t* curr = tiles; curr < block; ++curr) {
        overworld_render_tile(curr);
    }
    SC_PROFILE_END(render, overworld_render_tile);
    
    SC_PROFILE_START(render);
    for (overworld_tile_render_info_t* curr = tiles; curr < block; ++curr) {
        overworld_render_particles(curr, &camera->transform.position, pool);
    }
    SC_PROFILE_END(render, overworld_render_particles);
}