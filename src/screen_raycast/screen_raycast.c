#include "screen_raycast.h"

#include <libdragon.h>

struct screen_raycast_result {
    int id;
    int pixel_value;
};

typedef struct screen_raycast_result screen_raycast_result_t;

static screen_raycast_result_t raycast_state;
static void* output_position;

void screen_raycast_check_before_callback(void* pos) {
    if (!output_position) {
        return;
    }

    raycast_state.pixel_value = *(uint16_t*)output_position;
}

void screen_raycast_check_before(void* pos) {
    output_position = pos;
    rdpq_sync_full(screen_raycast_check_before_callback, pos);
}

void screen_raycast_check_before_after(void* pos) {
    if (!output_position) {
        return;
    }
    int pixel_value = *(uint16_t*)output_position;
    if (pixel_value != raycast_state.pixel_value) {
        raycast_state.id = (int)pos;
        raycast_state.pixel_value = pixel_value;
    }
}

void screen_raycast_check_after(int id) {
    rdpq_sync_full(screen_raycast_check_before_after, (void*)id);
}

void screen_raycast_read_result_deferred(void* data) {
    int* output = (int*)data;

    *output = raycast_state.id;
    raycast_state.id = 0;
}

void screen_raycast_read_result(int* output) {
    rdpq_call_deferred(screen_raycast_read_result_deferred, output);
}