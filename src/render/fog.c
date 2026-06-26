#include "fog.h"

#include "coloru8.h"
#include "../time/time.h"
#include "../math/mathf.h"

static fog_state_t fog_states[FOG_PRIORITY_COUNT];

static fog_state_t fog_start;
static fog_state_t fog_target;

static float fog_start_time;
static float fog_end_time;

fog_state_t fog_calculate_target() {
    for (int i = 0; i < FOG_PRIORITY_COUNT; i += 1) {
        if (fog_states[i].max == 0.0f && fog_states[i].min == 0.0f) {
            continue;
        }

        return fog_states[i];
    }

    return (fog_state_t){};
}

void fog_set(fog_priority_t priority, fog_state_t state, float duration) {
    fog_start = fog_get();
    fog_start_time = total_time;
    
    fog_states[priority] = state;

    fog_end_time = total_time + duration;
    fog_target = fog_calculate_target();
}

void fog_clear(fog_priority_t priority, float duration) {
    fog_start = fog_get();
    fog_start_time = total_time;
    
    fog_states[priority].max = 0.0f;
    fog_states[priority].min = 0.0f;

    fog_end_time = total_time + duration;
    fog_target = fog_calculate_target();
}

fog_state_t fog_get() {
    if (total_time >= fog_end_time) {
        return fog_target;
    }

    float lerp = (total_time - fog_start_time) / (fog_end_time - fog_start_time);

    return (fog_state_t){
        .color = coloru8_lerp(&fog_start.color, &fog_target.color, lerp),
        .min = mathfLerp(fog_start.min, fog_target.min, lerp),
        .max = mathfLerp(fog_start.max, fog_target.max, lerp),
    };
}