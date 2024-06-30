#include "dash_trail.h"

#include <malloc.h>
#include "../render/defs.h"
#include "../time/time.h"
#include "../math/mathf.h"
#include "../spell/assets.h"

void dash_trail_init(struct dash_trail* trail, struct Vector3* emit_from, bool flipped) {
    trail->first_time = 0;

    trail->first_vertex = 0;
    trail->vertex_count = 0;

    trail->flipped = flipped;
}

#define INITIAL_V           5.0f
#define TANGENT_VELOCITY_BOTTOM    1.5f
#define TANGENT_VELOCITY_TOP    2.5f
#define TANGENT_OFFSET      2.25f
#define GRAVITY             -9.8f

#define TIME_SPACING    0.1f

#define PREV_VERTEX(idx) ((idx) == 0 ? DASH_PARTICLE_COUNT - 1 : (idx) - 1)
#define NEXT_VERTEX(idx) ((idx) == DASH_PARTICLE_COUNT - 1 ? 0 : (idx) + 1)

void dash_trail_render(struct dash_trail* trail, struct Vector3* emit_from, struct render_batch* batch) {
    float particle_time = trail->first_time;

    trail->first_time += render_time_step;

    if (trail->first_time >= TIME_SPACING) {
        int start_index = trail->first_vertex;

        trail->first_time = fmodf(trail->first_time, TIME_SPACING);

        trail->first_vertex = PREV_VERTEX(trail->first_vertex);

        struct Vector3 offset;
        vector3Sub(emit_from, &trail->emit_from[start_index], &offset);

        if (trail->flipped) {
            vector3Negate(&offset, &offset);
        }

        vector3Cross(&offset, &gUp, &trail->tangent[trail->first_vertex]);
        vector3Normalize(&trail->tangent[trail->first_vertex], &trail->tangent[trail->first_vertex]);

        trail->emit_from[trail->first_vertex] = *emit_from;

        if (trail->vertex_count < DASH_PARTICLE_COUNT) {
            trail->vertex_count += 1;
        }
    }

    if (!trail->vertex_count) {
        return;
    }

    T3DVertPacked* vertices = frame_malloc(batch->pool, sizeof(T3DVertPacked) * trail->vertex_count);

    if (!vertices) {
        return;
    }

    T3DVertPacked* vertex = vertices;
    int current = trail->first_vertex;

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
        
        offset.y -= 2.0f * (1.0f / SCENE_SCALE);

        pack_position_vector(
            &offset, 
            vertex->posA
        );

        vector3AddScaled(&trail->emit_from[current], &trail->tangent[current], (particle_time * TANGENT_OFFSET) * TANGENT_VELOCITY_TOP, &offset);
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

    data_cache_hit_writeback_invalidate(vertices, sizeof(T3DVertPacked) * trail->vertex_count);

    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    rspq_block_begin();

    t3d_vert_load(vertices, 0, trail->vertex_count * 2);
    for (int i = 0; i < (trail->vertex_count - 1) * 2; i += 2) {
        t3d_tri_draw(i, i + 1, i + 2);
        t3d_tri_draw(i + 2, i + 1, i + 3);
    }

    t3d_tri_sync();

    rspq_block_t* block = rspq_block_end();;

    rspq_call_deferred((void (*)(void*))rspq_block_free, block);

    element->mesh.block = block;
    element->mesh.armature = NULL;
    element->mesh.transform = NULL;
    element->material = spell_assets_get()->dash_trail_material;
}

void dash_trail_destroy(struct dash_trail* trail) {
    
}