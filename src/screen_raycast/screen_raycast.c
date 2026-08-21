#include "screen_raycast.h"

#include <libdragon.h>

static uint32_t SCREEN_RAYCAST_ID = 0;
DEFINE_RSP_UCODE(rsp_screen_raycast);

void screen_raycast_init() {
    if (!SCREEN_RAYCAST_ID) {
        SCREEN_RAYCAST_ID = rspq_overlay_register(&rsp_screen_raycast);
    }
}

void screen_raycast_destroy() {
    if (SCREEN_RAYCAST_ID) {
        rspq_overlay_unregister(SCREEN_RAYCAST_ID);
        SCREEN_RAYCAST_ID = 0;
    }
}

void screen_raycast_check_pixel(void* pos) {
    assert(SCREEN_RAYCAST_ID);
    rspq_write(SCREEN_RAYCAST_ID, 0, PhysicalAddr(pos));
}

void screen_raycast_check_changed(void* pos, int id) {
    assert(SCREEN_RAYCAST_ID);
    rspq_write(SCREEN_RAYCAST_ID, 1, PhysicalAddr(pos), id);
}

void screen_raycast_read_entity(screen_raycast_result_t* result) {
    assert(SCREEN_RAYCAST_ID);
    rspq_write(SCREEN_RAYCAST_ID, 2, PhysicalAddr(result));

}