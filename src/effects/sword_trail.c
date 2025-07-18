#include "sword_trail.h"

#include "../render/defs.h"
#include "../time/time.h"
#include "./effect_allocator.h"
#include "../render/render_scene.h"
#include "../spell/assets.h"

#define PREV_VERTEX(index)  ((index) == 0 ? MAX_SWORD_TRAIL_LENGTH - 1 : (index) - 1)

void sword_trail_render_callback(void* data, struct render_batch* batch) {
    struct sword_trail* trail = (struct sword_trail*)data;

    if (trail->vertex_count <= 1) {
        return;
    }

    T3DVertPacked* vertices = frame_malloc(batch->pool, sizeof(T3DVertPacked) * trail->vertex_count);

    if (!vertices) {
        return;
    }

    int curr = PREV_VERTEX(trail->last_vertex);

    struct Vector3 effect_origin;

    for (int i = 0; i < trail->vertex_count; i += 1) {
        struct Vector3* input = trail->trail_halves[curr];

        if (i == 0) {
            effect_origin = input[0];
        }

        T3DVertPacked* output = &vertices[i];

        output->normA = 0;
        output->normB = 0;
        output->rgbaA = 0xFFFFFFFF;
        output->rgbaB = 0xFFFFFFFF;
        output->stA[0] = 0;
        output->stA[1] = 0;
        output->stB[0] = 0;
        output->stB[1] = 0;

        struct Vector3 relative;
        vector3Sub(&input[0], &effect_origin, &relative);
        pack_position_vector(
            &relative, 
            output->posA
        );

        vector3Sub(&input[1], &effect_origin, &relative);
        pack_position_vector(
            &relative, 
            output->posB
        );

        curr = PREV_VERTEX(curr);
    }

    T3DMat4FP* offset_mtx = render_batch_get_transformfp(batch);

    if (!offset_mtx) {
        return;
    }

    mat4x4 mtx;
    matrixFromScale(mtx, MODEL_WORLD_SCALE);
    matrixApplyScaledPos(mtx, &effect_origin, WORLD_SCALE);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(offset_mtx, (T3DMat4*)mtx);

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

void sword_trail_render(void* data, struct render_batch* batch) {
    render_batch_add_callback(batch, spell_assets_get()->dash_trail_material, sword_trail_render_callback, data);
}

void sword_trail_update(void* data) {
    struct sword_trail* trail = (struct sword_trail*)data;

    if (trail->vertex_count > 0) {
        --trail->vertex_count;
    } else if (trail->vertex_count == 0) {
        update_remove(trail);
        render_scene_remove(trail);
        effect_free(trail);
        return;
    }
}

struct sword_trail* sword_trail_new() {
    struct sword_trail* trail = effect_malloc(sizeof(struct sword_trail));

    if (!trail) {
        return NULL;
    }

    trail->last_vertex = 0;
    trail->vertex_count = 0;

    render_scene_add(&trail->trail_halves[0][0], 1.0f, sword_trail_render, trail);

    trail->trail_halves[0][0] = gZeroVec;

    return trail;
}

// NULL indicates the trial is done
void sword_trail_move(struct sword_trail* trail, struct Vector3* a, struct Vector3* b) {
    if (!trail) {
        return;
    }

    trail->trail_halves[trail->last_vertex][0] = *a;
    trail->trail_halves[trail->last_vertex][1] = *b;

    ++trail->last_vertex;

    if (trail->last_vertex == MAX_SWORD_TRAIL_LENGTH) {
        trail->last_vertex = 0;
    }

    if (trail->vertex_count < MAX_SWORD_TRAIL_LENGTH) {
        ++trail->vertex_count;
    }
}

void sword_trail_stop(struct sword_trail* trail) {
    if (!trail) {
        return;
    }

    update_add(trail, sword_trail_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}