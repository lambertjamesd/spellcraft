#include "water.h"

#include <libdragon.h>

static uint32_t WATER_OVERLAY_ID = 0;
static uint8_t simulation_count = 0;

#define PROCESS_BLOCK       0
#define PROCESS_PROFILE     1
#define PROCESS_APPLY       2

#define SIM_BUFFER_SIZE 1024

DEFINE_RSP_UCODE(rsp_water);

void water_simulation_init(water_simulation_t* simulation, int width, int height) {
    if (!simulation_count) {
        WATER_OVERLAY_ID = rspq_overlay_register(&rsp_water);
    }

    assert(width % 8 == 0);

    ++simulation_count;

    simulation->width = width;
    simulation->height = height;

    int pixel_count = width * height;
    int padded_pixel_count = pixel_count + width * 2;
    int total_size = 2 * padded_pixel_count * sizeof(uint8_t) + pixel_count * sizeof(uint16_t);

    simulation->velocity_buffer = malloc(total_size);
    simulation->position_buffers[0] = (int8_t*)(simulation->velocity_buffer + pixel_count);
    simulation->position_buffers[1] = simulation->position_buffers[0] + padded_pixel_count;

    simulation->read_buffer = 0;
    simulation->y_stride = SIM_BUFFER_SIZE / (width * sizeof(int16_t)) - 1;

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
    int8_t* in = simulation->position_buffers[simulation->read_buffer];
    int8_t* out = simulation->position_buffers[write_index];

    int block_y_stride = simulation->width * simulation->y_stride;
    int simluation_stride = simulation->width * sizeof(int8_t);
    
    for (int y = 1; y + 1 < simulation->height; y += simulation->y_stride) {
        int y_count = simulation->y_stride;
        int rows_remaining = simulation->height - y - 1;

        if (y_count > rows_remaining) {
            y_count = rows_remaining;
        }

        rspq_write(
            WATER_OVERLAY_ID, 
            PROCESS_BLOCK, 
            ((int)y_count << Y_STRIDE_OFFSET) | simluation_stride, 
            PhysicalAddr(vel), 
            PhysicalAddr(in), 
            PhysicalAddr(out + simluation_stride)
        );

        vel += block_y_stride;
        in += block_y_stride;
        out += block_y_stride;
    }

    simulation->read_buffer = write_index;
}

void water_simulation_apply(water_simulation_t* simulation, water_apply_args_t* args) {
    rspq_write_t write = rspq_write_begin(WATER_OVERLAY_ID, PROCESS_APPLY, 6);

    rspq_write_arg(&write, args->index_range_count);
    rspq_write_arg(&write, (int)PhysicalAddr(simulation->position_buffers[simulation->read_buffer]));
    rspq_write_arg(&write, PhysicalAddr(args->vtx));
    rspq_write_arg(&write, PhysicalAddr(args->index_ranges));
    rspq_write_arg(&write, args->min.equalTest);
    rspq_write_arg(&write, args->scale.equalTest);

    rspq_write_end(&write);
}