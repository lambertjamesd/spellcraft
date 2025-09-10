#ifndef __SPELL_LIGHTNING_STRIKE_H__
#define __SPELL_LIGHTNING_STRIKE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../render/render_batch.h"
#include <stdbool.h>

struct lightning_strike {
    struct Vector3 position;
    float timer;
};

typedef struct lightning_strike lightning_strike_t;

void lightning_strike_start(struct lightning_strike* strike, struct Vector3* position);
bool lightning_strike_update(struct lightning_strike* strike);
void lightning_strike_render(struct lightning_strike* strike, render_batch_t* batch);
bool lightning_strike_is_active(struct lightning_strike* strike);
float lightning_strike_cloud_size(struct lightning_strike* strike);

#endif