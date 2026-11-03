#ifndef __RENDER_COLOR_RAMP__
#define __RENDER_COLOR_RAMP__

#include <libdragon.h>

struct color_ramp_keyframe {
    color_t color;
    uint16_t time;
};

typedef struct color_ramp_keyframe color_ramp_keyframe_t;

struct color_ramp {
    uint16_t keyframe_count;
    uint16_t max_time;
    color_ramp_keyframe_t* keyframes;
};

typedef struct color_ramp color_ramp_t;

color_t color_ramp_eval(color_ramp_t* ramp, uint16_t time);

void color_ramp_load(color_ramp_t* ramp, FILE* file);
void color_ramp_free(color_ramp_t* ramp);

#endif