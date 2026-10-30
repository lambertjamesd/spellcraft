#ifndef __UTIL_LERP_H__
#define __UTIL_LERP_H__

static inline float lerp_ease_in(float input) {
    input = 1.0f - input;
    return 1.0f - input * input;
}

static inline float lerp_ease_out(float input) {
    return input * input;
}

static inline float lerp_ease_in_out(float input) {
    return input * input * (3.0f - 2.0f * input);
}

#endif