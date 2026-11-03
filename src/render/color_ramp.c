#include "color_ramp.h"

#include "coloru8.h"

color_t color_ramp_eval(color_ramp_t* ramp, uint16_t time) {
    int index = 0;

    color_ramp_keyframe_t* curr = ramp->keyframes;
    color_ramp_keyframe_t* end = curr + ramp->keyframe_count - 1;

    while (curr < end && time > curr->time) {
        ++curr;
    }

    if (curr == end && time >= end->time) {
        return end->color;
    }

    if (curr == ramp->keyframes) {
        return ramp->keyframes->color;
    }

    color_ramp_keyframe_t* prev = curr - 1;
    uint16_t offset = curr->time - prev->time;

    uint16_t relative_time = time - prev->time;

    return coloru8_lerp(&prev->color, &curr->color, (float)relative_time / (float)offset);
}

void color_ramp_load(color_ramp_t* ramp, FILE* file) {
    fread(&ramp->keyframe_count, 2, 1, file);
    fread(&ramp->max_time, 2, 1, file);

    if (!ramp->keyframe_count) {
        ramp->keyframes = NULL;
        return;
    }

    ramp->keyframes = malloc(sizeof(color_ramp_keyframe_t) * ramp->keyframe_count);
    fread(ramp->keyframes, sizeof(color_ramp_keyframe_t), ramp->keyframe_count, file);
}

void color_ramp_free(color_ramp_t* ramp) {
    free(ramp->keyframes);
    ramp->keyframes = NULL;
}