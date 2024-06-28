
#include "mana_regulator.h"
#include "../time/time.h"
#include <math.h>

void mana_regulator_init(struct mana_regulator* regulator, float burst_mana) {
    regulator->burst_mana = burst_mana;
    regulator->burst_mana_rate = sqrtf(burst_mana);
}

float mana_regulator_request(struct mana_regulator* regulator, struct mana_pool* fallback, float amount) {
    float result = mana_pool_request(fallback, amount);

    if (regulator->burst_mana > 0.0f) {
        float additional = fixed_time_step * regulator->burst_mana_rate;
        if (additional > regulator->burst_mana) {
            additional = regulator->burst_mana;
            regulator->burst_mana = 0.0f;
        } else {
            regulator->burst_mana -= additional;
        }

        result += additional;
    }

    return result;
}