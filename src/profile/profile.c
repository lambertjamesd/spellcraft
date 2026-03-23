#include "profile.h"

#include <libdragon.h>

#define MAX_DEPTH   8

uint64_t start_times[MAX_DEPTH];
uint8_t current_depth;

void profile_start() {
    assert(current_depth < MAX_DEPTH);
    start_times[current_depth] = get_ticks_us();
    ++current_depth;
}

void profile_end(const char* name) {
    assert(current_depth > 0);
    --current_depth;
    debugf("%s: %f\n", name, (get_ticks_us() - start_times[current_depth]) * (1.0f / 1000.0f));
}