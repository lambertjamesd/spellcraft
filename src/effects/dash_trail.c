#include "dash_trail.h"

#include "../render/defs.h"
#include "../time/time.h"
#include "../math/mathf.h"
#include "../spell/assets.h"
#include "./effect_allocator.h"
#include "../render/render_scene.h"

#define INITIAL_V           5.0f
#define TANGENT_VELOCITY_BOTTOM    1.5f
#define TANGENT_VELOCITY_TOP    2.5f
#define TANGENT_OFFSET      2.25f
#define GRAVITY             -9.8f

#define TIME_SPACING    0.1f

#define PREV_VERTEX(idx) ((idx) == 0 ? DASH_PARTICLE_COUNT - 1 : (idx) - 1)
#define NEXT_VERTEX(idx) ((idx) == DASH_PARTICLE_COUNT - 1 ? 0 : (idx) + 1)

void dash_trail_render_callback(void* data, struct render_batch* batch) {
    struct dash_trail* trail = (struct dash_trail*)data;

    if (!trail->vertex_count) {
        return;
    }

    T3DMat4FP* offset_mtx = render_batch_get_transformfp(batch);

    if (!offset_mtx) {
        return;
    }

    mat4x4 mtx;
    struct Vector3 effect_origin = {
        .x = trail->last_position.x - batch->camera_matrix[3][0],
        .y = trail->last_position.y - batch->camera_matrix[3][1],
        .z = trail->last_position.z - batch->camera_matrix[3][2],
    };
    matrixFromScale(mtx, MODEL_WORLD_SCALE);
    matrixApplyScaledPos(mtx, &effect_origin, WORLD_SCALE);
    t3d_mat4_to_fixed_3x4(offset_mtx, (T3DMat4*)mtx);

    T3DVertPacked* vertices = frame_malloc(batch->pool, sizeof(T3DVertPacked) * trail->vertex_count);

    if (!vertices) {
        return;
    }

    T3DVertPacked* vertex = vertices;
    int current = trail->first_vertex;
    float particle_time = trail->first_time;

    for (int i = 0; i < trail->vertex_count; i += 1) {
        int vertex_index = i * 2;

        vertex->normA = 0;
        vertex->normB = 0;
        vertex->rgbaA = 0xFFFFFFFF;
        vertex->rgbaB = 0xFFFFFFFF;
        vertex->stA[0] = 0;
        vertex->stA[1] = 0;
        vertex->stB[0] = 0;
        vertex->stB[1] = 0;

        struct Vector3 offset;
        vector3AddScaled(&trail->emit_from[current], &trail->tangent[current], (particle_time * TANGENT_OFFSET) * TANGENT_VELOCITY_BOTTOM, &offset);
        vector3Sub(&offset, &trail->last_position, &offset);

        offset.y -= 2.0f * MODEL_WORLD_SCALE;

        pack_position_vector(
            &offset, 
            vertex->posA
        );

        vector3AddScaled(&trail->emit_from[current], &trail->tangent[current], (particle_time * TANGENT_OFFSET) * TANGENT_VELOCITY_TOP, &offset);
        vector3Sub(&offset, &trail->last_position, &offset);
        float vertical_jump = particle_time * INITIAL_V + particle_time * particle_time * GRAVITY;
        if (vertical_jump >= 0) {
            offset.y += vertical_jump;
        } else {
            trail->vertex_count = i;
            break;
        }

        pack_position_vector(
            &offset, 
            vertex->posB
        );

        current = NEXT_VERTEX(current);

        particle_time += TIME_SPACING;
        vertex += 1;
    }

    if (!trail->vertex_count) {
        return;
    }

    t3d_matrix_push(offset_mtx);

    data_cache_hit_writeback_invalidate(vertices, sizeof(T3DVertPacked) * trail->vertex_count);

    t3d_vert_load(vertices, 0, trail->vertex_count * 2);
    
    for (int i = 0; i < (trail->vertex_count - 1) * 2; i += 2) {
        t3d_tri_draw(i, i + 1, i + 2);
        t3d_tri_draw(i + 2, i + 1, i + 3);
    }

    t3d_tri_sync();

    t3d_matrix_pop(1);
}

void dash_trail_render(void* data, struct render_batch* batch) {
    render_batch_add_callback(batch, spell_assets_get()->dash_trail_material, dash_trail_render_callback, data);
}

void dash_trail_update(void* data) {
    struct dash_trail* trail = (struct dash_trail*)data;

    if (trail->vertex_count == 0 && !trail->active) {
        update_remove(trail);
        render_scene_remove(trail);
        effect_free(trail);
        return;
    }

    trail->first_time += render_time_step;

    if (trail->first_time >= TIME_SPACING && trail->active) {
        int start_index = trail->first_vertex;

        trail->first_time = fmodf(trail->first_time, TIME_SPACING);

        trail->first_vertex = PREV_VERTEX(trail->first_vertex);

        struct Vector3 offset;
        vector3Sub(&trail->last_position, &trail->emit_from[start_index], &offset);

        if (trail->flipped) {
            vector3Negate(&offset, &offset);
        }

        vector3Cross(&offset, &gUp, &trail->tangent[trail->first_vertex]);
        vector3Normalize(&trail->tangent[trail->first_vertex], &trail->tangent[trail->first_vertex]);

        trail->emit_from[trail->first_vertex] = trail->last_position;

        if (trail->vertex_count < DASH_PARTICLE_COUNT) {
            trail->vertex_count += 1;
        }
    }
}

struct dash_trail* dash_trail_new(struct Vector3* emit_from, bool flipped) {
    struct dash_trail* trail = effect_malloc(sizeof(struct dash_trail));

    if (!trail) {
        return NULL;
    }

    trail->first_time = 0;

    trail->first_vertex = 0;
    trail->vertex_count = 0;

    trail->flipped = flipped;

    trail->emit_from[0] = *emit_from;
    trail->tangent[0] = gZeroVec;

    trail->last_position = *emit_from;
    trail->active = 1;

    render_scene_add(&trail->last_position, 4.0f, dash_trail_render, trail);
    update_add(trail, dash_trail_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    dash_trail_move(trail, emit_from);

    return trail;
}

void dash_trail_move(struct dash_trail* trail, struct Vector3* emit_from) {
    if (emit_from) {
        trail->last_position = *emit_from;
    } else {
        trail->active = 0;
    }
}