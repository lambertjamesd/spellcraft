
#include "mana_regulator.h"
#include "../time/time.h"
#include <math.h>

void mana_regulator_init(struct mana_regulator* regulator, float burst_mana, float rate_ratio) {
    regulator->burst_mana = burst_mana;
    regulator->burst_mana_rate = sqrtf(burst_mana) * rate_ratio;
}

float mana_regulator_request(struct mana_regulator* regulator, struct mana_pool* fallback, float amount) {
    float result = 0.0f;
    
    if (regulator->burst_mana > 0.0f) {
        float additional = scaled_time_step * regulator->burst_mana_rate;
        if (additional > regulator->burst_mana) {
            additional = regulator->burst_mana;
            regulator->burst_mana = 0.0f;
        } else {
            regulator->burst_mana -= additional;
        }

        result = additional;

        if (amount <= additional) {
            return additional;
        }

        amount -= additional;
    }

    if (!fallback) {
        return result;
    }

    return result + mana_pool_request(fallback, amount);;
}