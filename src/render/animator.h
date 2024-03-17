#ifndef __RENDER_ANIMATIONS_H__
#define __RENDER_ANIMATIONS_H__

#include <stdint.h>

struct animation_clip {
    uint16_t bone_count;
    uint16_t tick_count;
    uint16_t ticks_per_second;
};

struct animator {
    uint16_t bone_count;
    struct animation_clip* clip;
};

#endif