#ifndef __RENDER_ANIMATIONS_H__
#define __RENDER_ANIMATIONS_H__

#include <stdint.h>

struct animator {
    uint16_t bone_count;
    struct animation_clip* clip;
};

#endif