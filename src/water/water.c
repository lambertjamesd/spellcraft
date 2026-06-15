#include "water.h"

#include <libdragon.h>

static uint32_t WATER_OVERLAY_ID = 0;
static uint8_t simulation_count = 0;

#define PROCESS_BLOCK   0

#define SIM_BUFFER_SIZE 1024

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
    // simulation->y_stride = SIM_BUFFER_SIZE / (width * sizeof(int16_t));
    simulation->y_stride = 1;

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

// 12 is chosen to simplify calculating dma transfer sizes
#define Y_STRIDE_OFFSET 12

void water_simulation_update(water_simulation_t* simulation) {
    int write_index = 1 - simulation->read_buffer;

    int16_t* vel = simulation->velocity_buffer;
    int16_t* in = simulation->position_buffers[simulation->read_buffer];
    int16_t* out = simulation->position_buffers[write_index];

    int block_y_stride = simulation->width * simulation->y_stride;
    int simluation_stride = simulation->width * sizeof(int16_t);
    
    for (int y = 1; y + 1 < simulation->height; y += simulation->y_stride) {
        int y_count = simulation->y_stride;
        int rows_remaining = simulation->height - y - 1;

        if (y_count > rows_remaining) {
            y_count = rows_remaining;
        }

        rspq_write(WATER_OVERLAY_ID, PROCESS_BLOCK,  ((int)y_count << Y_STRIDE_OFFSET) | simluation_stride, PhysicalAddr(vel), PhysicalAddr(in), PhysicalAddr(out));

        vel += block_y_stride;
        in += block_y_stride;
        out += block_y_stride;
    }

    simulation->read_buffer = write_index;
}