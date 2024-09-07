#ifndef __EFFECTS_PARTICLE_SPRAY_H__
#define __EFFECTS_PARTICLE_SPRAY_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../render/material.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_PARTICLE_COUNT  8

struct particle_spray {
    struct Vector3 particle_offset[MAX_PARTICLE_COUNT];
    struct Vector3 position;
    struct Vector3 direction;
    float cycle_time;
    float total_time;
    float end_time;
    struct material* material;
    uint16_t index_offset;
};

#endif
