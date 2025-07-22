
#include "mana_pool.h"
#include "../time/time.h"
#include "../util/hash_map.h"
#include "../math/minmax.h"

#define MANA_REGEN_DELAY    0.5f
#define MANA_LEAK_RATE      0.03f

#define MANA_PREV_THRESHOLD 0.1f
#define MANA_PREV_FALL_RATE 10.0f

static struct hash_map mana_entity_mapping;

void mana_pool_reset() {
    hash_map_destroy(&mana_entity_mapping);
    hash_map_init(&mana_entity_mapping, 32);
}

void mana_pool_init(struct mana_pool* pool, struct mana_pool_definition* definition) {
    pool->max_mana = definition->max_mana;
    pool->current_mana = definition->max_mana;
    pool->charged_mana = 0.0f;
    pool->previous_mana = 0.0f;
    pool->mana_regen_rate = definition->mana_regen_rate;
    pool->power_scale = definition->power_scale;
    pool->last_request_time = 0.0f;
}

void mana_pool_update(struct mana_pool* pool) {
    float time_to_last_cast = game_time - pool->last_request_time;

    if (pool->charged_mana > 0.0f) {
        if (pool->charged_mana < 0.1f) {
            pool->current_mana += pool->charged_mana;
            pool->charged_mana = 0.0f;
        } else {
            float leak_amount = pool->charged_mana * MANA_LEAK_RATE;
            pool->current_mana += leak_amount;
            pool->charged_mana -= leak_amount;
        }
    } else if (time_to_last_cast > MANA_REGEN_DELAY) {
        pool->current_mana += pool->mana_regen_rate * fixed_time_step;

        if (pool->current_mana > pool->max_mana) {
            pool->current_mana = pool->max_mana;
        }
    } 

    if (time_to_last_cast > MANA_PREV_THRESHOLD) {
        pool->previous_mana -= MANA_PREV_FALL_RATE * fixed_time_step;
    }
    
    if (pool->previous_mana < pool->current_mana) {
        pool->previous_mana = pool->current_mana;
    }
}

void mana_pool_charge(struct mana_pool* pool, float amount) {
    pool->charged_mana += mana_pool_request(pool, amount);
}

void mana_pool_add(struct mana_pool* pool, float amount) {
    if (!pool) {
        return;
    }
    pool->current_mana = MIN(pool->max_mana, pool->current_mana + amount);
}

float mana_pool_request(struct mana_pool* pool, float amount) {
    float scale_amount = amount * pool->power_scale;

    // max_mana of 0 is infinite mana
    if (pool->max_mana == 0.0f) {
        return scale_amount;
    }

    if (game_time - pool->last_request_time > MANA_PREV_THRESHOLD) {
        pool->previous_mana = pool->current_mana;
    }

    if (scale_amount > pool->current_mana) {
        scale_amount = pool->current_mana;
        pool->current_mana = 0.0f;
    } else {
        pool->current_mana -= scale_amount;
    }

    pool->last_request_time = game_time;

    return scale_amount;
}

float mana_pool_request_ratio(struct mana_pool* pool, float amount) {
    // max_mana of 0 is infinite mana
    if (pool->max_mana == 0.0f) {
        return pool->power_scale;
    }

    float scale_amount = amount * pool->power_scale;
    float result = pool->power_scale;

    if (game_time - pool->last_request_time > MANA_PREV_THRESHOLD) {
        pool->previous_mana = pool->current_mana;
    }

    if (scale_amount > pool->current_mana) {
        result = pool->current_mana / amount;
        pool->current_mana = 0.0f;
    } else {
        pool->current_mana -= scale_amount;
    }

    pool->last_request_time = game_time;

    return result;
}

float mana_pool_get_previous_mana(struct mana_pool* pool) {
    if (pool->charged_mana > 0.0f) {
        return pool->current_mana + pool->charged_mana;
    }

    return pool->previous_mana;
}

float mana_pool_request_charged_mana(struct mana_pool* pool) {
    float result = pool->charged_mana;

    pool->charged_mana = 0.0f;
    pool->previous_mana = pool->current_mana;

    return result;
}

void mana_pool_set_entity_id(struct mana_pool* pool, entity_id entity_id) {
    hash_map_set(&mana_entity_mapping, entity_id, pool);
}

void mana_pool_clear_entity_id(entity_id entity_id) {
    hash_map_delete(&mana_entity_mapping, entity_id);
}

struct mana_pool* mana_pool_get(entity_id entity_id) {
    return hash_map_get(&mana_entity_mapping, entity_id);
}