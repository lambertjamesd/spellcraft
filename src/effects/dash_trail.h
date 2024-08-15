#ifndef __EFFECTS_DASH_TRAIL_H__
#define __EFFECTS_DASH_TRAIL_H__

#include "../math/vector3.h"
#include "../render/render_batch.h"
#include <t3d/t3d.h>

#define DASH_PARTICLE_COUNT 16

struct dash_trail {
    struct Vector3 emit_from[DASH_PARTICLE_COUNT];
    struct Vector3 tangent[DASH_PARTICLE_COUNT];

    struct Vector3 last_position;

    float first_time;

    uint16_t flipped: 1;
    uint16_t active: 1;

    uint16_t first_vertex;
    uint16_t vertex_count;
};

struct dash_trail* dash_trail_new(struct Vector3* emit_from, bool flipped);
// NULL indicates the trial is done
void dash_trail_move(struct dash_trail* trail, struct Vector3* emit_from);

#endif