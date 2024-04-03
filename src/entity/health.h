#ifndef __ENTITY_HEALTH_H__
#define __ENTITY_HEALTH_H__

#include "entity_id.h"
#include <stdint.h>

enum damage_type {
    DAMAGE_TYPE_PROJECTILE = (1 << 0),
    DAMAGE_TYPE_FIRE = (1 << 1),
    DAMAGE_TYPE_ICE = (1 << 2),
    DAMAGE_TYPE_BASH = (1 << 3),
};

struct health {
    entity_id entity_id;
    float max_health;
    float current_health;
    uint16_t flaming_timer;
    uint16_t icy_timer;
};

void health_reset();

void health_init(struct health* health, entity_id id, float max_health);
void health_destroy(struct health* health);

void health_damage(struct health* health, float amount, entity_id source, enum damage_type type);

struct health* health_get(entity_id id);

#endif