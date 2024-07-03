#ifndef __MANA_REGULATOR_H__
#define __MANA_REGULATOR_H__

#include "mana_pool.h"

struct mana_regulator {
    float burst_mana;
    float burst_mana_rate;
};

void mana_regulator_init(struct mana_regulator* regulator, float burst_mana, float rate_ratio);
float mana_regulator_request(struct mana_regulator* regulator, struct mana_pool* fallback, float rate);

#endif