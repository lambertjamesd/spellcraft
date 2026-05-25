#include "water.h"

#include <libdragon.h>

static uint32_t WATER_OVERLAY_ID = 0;
static uint8_t simulation_count = 0;

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

    memset(simulation->velocity_buffer, 0, pixel_count * total_size);
}

void water_simulation_destroy(water_simulation_t* simulation) {
    --simulation_count;

    if (!simulation_count) {
        rspq_overlay_unregister(WATER_OVERLAY_ID);
    }

    free(simulation->velocity_buffer);
    simulation->velocity_buffer = NULL;
}

void water_simulation_update(water_simulation_t* simulation) {

}