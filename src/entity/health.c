#include "health.h"

#include "../util/hash_map.h"

static struct hash_map health_entity_mapping;

void health_reset() {
    hash_map_destroy(&health_entity_mapping);
    hash_map_init(&health_entity_mapping, 32);
}

void health_init(struct health* health, entity_id id, float max_health) {
    health->entity_id = id;
    health->max_health = max_health;
    health->current_health = max_health;

    hash_map_set(&health_entity_mapping, id, health);
}

void health_destroy(struct health* health) {
    hash_map_delete(&health_entity_mapping, health->entity_id);
}