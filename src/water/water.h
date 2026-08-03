#ifndef __WATER_WATER_H__
#define __WATER_WATER_H__

#include <stdint.h>
#include <t3d/t3d.h>
#include "../math/vector2s16.h"

struct water_simulation {
    uint16_t width;
    uint16_t height;
    int16_t* velocity_buffer;
    int8_t* position_buffers[2];
    uint8_t read_buffer;
    uint8_t y_stride;
};

typedef struct water_simulation water_simulation_t;

struct water_index_range {
    uint16_t min, max;
};

typedef struct water_index_range water_index_range_t;

struct water_apply_args {
    T3DVertPacked* vtx;
    water_index_range_t* index_ranges;
    vector2s16_t min;
    vector2s16_t scale;
    uint16_t index_range_count;
};

typedef struct water_apply_args water_apply_args_t;

void water_simulation_init(water_simulation_t* simulation, int width, int height);

void water_simulation_update(water_simulation_t* simulation);

void water_simulation_destroy(water_simulation_t* simulation);

void water_simulation_apply(water_simulation_t* simulation, water_apply_args_t* args);

#endif