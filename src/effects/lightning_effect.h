#ifndef __LIGHTNING_EFFECT_H__
#define __LIGHTNING_EFFECT_H__

#include "../math/vector3.h"
#include "../render/armature.h"

struct lightning_effect_def {
    float spread;
};

struct lightning_effect {
    struct armature armature;
    struct lightning_effect_def* def;
    struct Vector3 last_position;
    short next_transform;
};

struct lightning_effect* lightning_effect_new(struct Vector3* position, struct lightning_effect_def* def);
void lightning_effect_set_position(struct lightning_effect* effect, struct Vector3* position, struct Vector3* direction, float radius);
void lightning_effect_free(struct lightning_effect*);

#endif