#include "dash_trail.h"

#include <malloc.h>
#include "../render/defs.h"
#include "../time/time.h"
#include "../math/mathf.h"

void dash_trail_init(struct dash_trail* trail, struct Vector3* emit_from, bool flipped) {
    trail->vertices = malloc(sizeof(T3DVertPacked) * DASH_PARTICLE_COUNT);

    for (int i = 0; i < DASH_PARTICLE_COUNT; i += 1) {
        trail->vertices[i].normA = 0;
        pack_position_vector(emit_from, trail->vertices[i].posA);
        trail->vertices[i].normB = 0;
        pack_position_vector(emit_from, trail->vertices[i].posB);

        trail->vertices[i].rgbaA = 0xFFFFFFFF;
        trail->vertices[i].rgbaB = 0xFFFFFFFF;

        trail->vertices[i].stA[0] = 0;
        trail->vertices[i].stA[1] = 0;

        trail->vertices[i].stB[0] = 0;
        trail->vertices[i].stB[1] = 0;

        trail->emit_from[i] = *emit_from;
        trail->tangent[i] = gZeroVec;
    }

    data_cache_hit_writeback_invalidate(trail->vertices, sizeof(T3DVertPacked) * DASH_PARTICLE_COUNT);

    rspq_block_begin();

    t3d_vert_load(trail->vertices, 0, DASH_PARTICLE_COUNT * 2);

    for (int i = 0; i < (DASH_PARTICLE_COUNT - 2) * 2; i += 2) {
        t3d_tri_draw(i, i + 1, i + 2);
        t3d_tri_draw(i + 2, i + 1, i + 3);
    }

    t3d_tri_sync();

    trail->render_block = rspq_block_end();
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

void dash_trail_update(struct dash_trail* trail, struct Vector3* emit_from) {
    int current = trail->first_vertex;
    float particle_time = trail->first_time;

    for (int i = 0; i < trail->vertex_count; i += 1) {
        int vertex_index = i * 2;

        struct Vector3 offset;
        vector3AddScaled(&trail->emit_from[current], &trail->tangent[current], (particle_time * TANGENT_OFFSET) * TANGENT_VELOCITY_BOTTOM, &offset);

        pack_position_vector(
            &offset, 
            t3d_vertbuffer_get_pos(trail->vertices, vertex_index)
        );

        vector3AddScaled(&trail->emit_from[current], &trail->tangent[current], (particle_time * TANGENT_OFFSET) * TANGENT_VELOCITY_TOP, &offset);
        float vertical_jump = particle_time * INITIAL_V + particle_time * particle_time * GRAVITY;
        if (vertical_jump > 0) {
            offset.y += vertical_jump;
        }

        pack_position_vector(
            &offset, 
            t3d_vertbuffer_get_pos(trail->vertices, vertex_index + 1)
        );

        current = NEXT_VERTEX(current);

        particle_time += TIME_SPACING;
    }

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

    data_cache_hit_writeback_invalidate(trail->vertices, sizeof(T3DVertPacked) * DASH_PARTICLE_COUNT);
}

void dash_trail_destroy(struct dash_trail* trail) {
    free(trail->vertices);
    trail->vertices = NULL;
    rspq_block_free(trail->render_block);
    trail->render_block = NULL;
}