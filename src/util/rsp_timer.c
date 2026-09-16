#include "rsp_timer.h"

#include <libdragon.h>

#define MAX_TIMERS  16

static uint32_t TIMER_OVERLAY_ID = 0;
static bool is_dirty = true;
static float cached_timers[MAX_TIMERS];

DEFINE_RSP_UCODE(rsp_timing);

#define TICKS_TO_SECONDS     (1.0f / 62500.0f)

void rsp_timer_init() {
    TIMER_OVERLAY_ID = rspq_overlay_register(&rsp_timing);
}

void rsp_timer_destroy() {
    rspq_overlay_unregister(TIMER_OVERLAY_ID);
}

void rsp_timer_start(unsigned index) {
    assert(index < MAX_TIMERS);
    rspq_write(TIMER_OVERLAY_ID, 0, index * 4);
    is_dirty = true;
}

void rsp_timer_end(unsigned index, timer_output_t* output) {
    assert(index < MAX_TIMERS);
    rspq_write(TIMER_OVERLAY_ID, 1, index * 4, PhysicalAddr(output));
    is_dirty = true;
}

float rsp_timer_output_ms(timer_output_t* output) {
    return ((timer_output_t*)UncachedAddr(output))->result * TICKS_TO_SECONDS;
}