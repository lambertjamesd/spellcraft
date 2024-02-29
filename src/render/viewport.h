#ifndef __RENDER_VIEWPORT_H__
#define __RENDER_VIEWPORT_H__

#include <stdint.h>

struct render_viewport {
    uint16_t x, y;
    uint16_t w, h;
};

#endif