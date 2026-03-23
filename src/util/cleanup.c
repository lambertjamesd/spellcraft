#include "./cleanup.h"

#include <libdragon.h>

static bool immediate = false;

void cleanup_safe(cleanup_callback callback, void* data) {
    if (immediate) {
        callback(data);
    } else {
        rspq_call_deferred(callback, data);
    }
}

void cleanup_set_immediate(bool value) {
    immediate = value;
}