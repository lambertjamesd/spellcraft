#ifndef __WATER_WATER_H__
#define __WATER_WATER_H__

#include <stdint.h>

struct water_simulation {
    uint16_t width;
    uint16_t height;
    int16_t* velocity_buffer;
    int16_t* position_buffers[2];
    uint8_t read_buffer;
    uint8_t y_stride;
};

typedef struct water_simulation water_simulation_t;

void water_simulation_init(water_simulation_t* simulation, int width, int height);

void water_simulation_update(water_simulation_t* simulation);

void water_simulation_destroy(water_simulation_t* simulation);

#endif