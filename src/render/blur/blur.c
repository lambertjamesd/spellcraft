#include "blur.h"

#include <libdragon.h>

static uint32_t BLUR_OVERLAY_ID = 0;
DEFINE_RSP_UCODE(rsp_blur);

void blur_init() {
    if (!BLUR_OVERLAY_ID) {
        BLUR_OVERLAY_ID = rspq_overlay_register(&rsp_blur);
    }
}

void blur_destroy() {
    if (BLUR_OVERLAY_ID) {
        rspq_overlay_unregister(BLUR_OVERLAY_ID);
        BLUR_OVERLAY_ID = 0;
    }
}

void blur_buffer(void* buffer, float strength) {
    assert(strength >= 0.0f && strength <= 1.0f);

    int blur_strength = (2.0f - strength) * 0x8000;

    if (blur_strength >= 0x10000) {
        return;
    }

    rspq_write(BLUR_OVERLAY_ID, 0, blur_strength, PhysicalAddr(buffer));
}