#ifndef __BURNING_EFFECT_H__
#define __BURNING_EFFECT_H__

#include "../math/vector3.h"
#include <stdbool.h>

struct burning_effect {
    struct Vector3 position;
    float size;
    float current_size;
    float current_time;
};

typedef struct burning_effect burning_effect_t;

struct burning_effect* burning_effect_new(struct Vector3* position, float size, float duration);
void bunring_effect_set_position(struct burning_effect* effect, struct Vector3* position);
void burning_effect_refresh(struct burning_effect* effect, float duration);
void burning_effect_stop(struct burning_effect* effect);

void burning_effect_free(struct burning_effect* effect);

bool burning_effect_is_active(struct burning_effect* effect);

#endif