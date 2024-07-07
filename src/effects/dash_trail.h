#ifndef __EFFECTS_DASH_TRAIL_H__
#define __EFFECTS_DASH_TRAIL_H__

#include "../math/vector3.h"
#include "../render/render_batch.h"
#include <t3d/t3d.h>

#define DASH_PARTICLE_COUNT 16

struct dash_trail {
    struct Vector3 emit_from[DASH_PARTICLE_COUNT];
    struct Vector3 tangent[DASH_PARTICLE_COUNT];

    float first_time;

    uint16_t flipped: 1;

    uint16_t first_vertex;
    uint16_t vertex_count;
};

void dash_trail_init(struct dash_trail* trail, struct Vector3* emit_from, bool flipped);
void dash_trail_render(struct dash_trail* trail, struct Vector3* emit_from, struct render_batch* batch);

void dash_trail_destroy(struct dash_trail* trail);

#endif