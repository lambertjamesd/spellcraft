#include "health.h"

#include "../util/hash_map.h"
#include "../time/time.h"
#include <stddef.h>

static struct hash_map health_entity_mapping;

void health_reset() {
    hash_map_destroy(&health_entity_mapping);
    hash_map_init(&health_entity_mapping, 32);
}

void health_update(void *data) {
    struct health* health = (struct health*)data;

    if (health->icy_timer) {
        health->icy_timer -= 1;
    }

    if (health->flaming_timer) {
        health->flaming_timer -= 1;
    }
}

void health_init(struct health* health, entity_id id, float max_health) {
    health->entity_id = id;
    health->max_health = max_health;
    health->current_health = max_health;

    hash_map_set(&health_entity_mapping, id, health);

    update_add(health, health_update, 0, UPDATE_LAYER_WORLD);

    health->callback = NULL;
    health->callback_data = NULL;
}

void health_destroy(struct health* health) {
    hash_map_delete(&health_entity_mapping, health->entity_id);
}

void health_damage(struct health* health, float amount, entity_id source, enum damage_type type) {
    if (health->max_health) {
        health->current_health -= amount;
    }

    if (type & DAMAGE_TYPE_FIRE) {
        health->flaming_timer = 30;
    }

    if (type & DAMAGE_TYPE_ICE) {
        health->icy_timer = 30;
    }

    if (health->callback) {
        health->callback(health->callback_data, amount, source, type);
    }
}

struct health* health_get(entity_id id) {
    return hash_map_get(&health_entity_mapping, id);
}