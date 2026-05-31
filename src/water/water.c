#include "water.h"

#include <libdragon.h>

static uint32_t WATER_OVERLAY_ID = 0;
static uint8_t simulation_count = 0;

#define PROCESS_BLOCK   0

DEFINE_RSP_UCODE(rsp_water);

void water_simulation_init(water_simulation_t* simulation, int width, int height) {
    if (!simulation_count) {
        WATER_OVERLAY_ID = rspq_overlay_register(&rsp_water);
    }

    ++simulation_count;

    simulation->width = width;
    simulation->height = height;

    int pixel_count = width * height;
    int total_size = sizeof(int16_t) * pixel_count * 3;

    simulation->velocity_buffer = malloc(total_size);
    simulation->position_buffers[0] = simulation->velocity_buffer + pixel_count;
    simulation->position_buffers[1] = simulation->position_buffers[0] + pixel_count;

    simulation->read_buffer = 0;

    memset(simulation->velocity_buffer, 0, total_size);
}

void water_simulation_destroy(water_simulation_t* simulation) {
    --simulation_count;

    if (!simulation_count) {
        rspq_overlay_unregister(WATER_OVERLAY_ID);
    }

    free(simulation->velocity_buffer);
    simulation->velocity_buffer = NULL;
}

#define X_STRIDE    6
#define Y_STRIDE    6

void water_simulation_update(water_simulation_t* simulation) {
    int write_index = 1 - simulation->read_buffer;

    int16_t* vel = simulation->velocity_buffer;
    int16_t* in = simulation->position_buffers[simulation->read_buffer];
    int16_t* out = simulation->position_buffers[write_index];

    int block_y_stride = simulation->width * Y_STRIDE;
    int simluation_stride = simulation->width * sizeof(int16_t);
    
    for (int y = 0; y + Y_STRIDE+1 < simulation->height; y += Y_STRIDE) {
        int16_t* vel_x = vel;
        int16_t* in_x = in;
        int16_t* out_x = out;

        for (int x = 0; x + X_STRIDE+1 < 8; x += X_STRIDE) {    
            rspq_write(WATER_OVERLAY_ID, PROCESS_BLOCK, simluation_stride, PhysicalAddr(vel_x), PhysicalAddr(in_x), PhysicalAddr(out_x));

            vel_x += X_STRIDE;
            in_x += X_STRIDE;
            out_x += X_STRIDE;
        }

        vel += block_y_stride;
        in += block_y_stride;
        out += block_y_stride;
    }

    simulation->read_buffer = write_index;
}