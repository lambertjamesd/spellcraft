#include "blur.h"

#include <libdragon.h>
#include "../../math/mathf.h"

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
    int blur_strength = 0x10000 * clampf(1.0f - strength, 0.0f, 1.0f);

    if (blur_strength >= 0x10000) {
        return;
    }

    rspq_write(BLUR_OVERLAY_ID, 0, blur_strength & 0xFFFF, PhysicalAddr(buffer));
}