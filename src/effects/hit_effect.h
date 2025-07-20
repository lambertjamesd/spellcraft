#ifndef __EFFECTS_HIT_EFFECT_H__
#define __EFFECTS_HIT_EFFECT_H__

#include "../math/transform.h"

struct hit_effect {
    struct Transform transform;
    float timer;
};

void hit_effect_start(struct Vector3* position, struct Vector3* normal);

#endif