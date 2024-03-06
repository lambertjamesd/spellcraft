#ifndef __ENTITY_HEALTH_H__
#define __ENTITY_HEALTH_H__

#include "entity_id.h"

struct health {
    entity_id entity_id;
    float max_health;
    float current_health;
};

void health_reset();

void health_init(struct health* health, entity_id id, float max_health);
void health_destroy(struct health* health);

void health_damage(struct health* health, float amount, entity_id source);

struct health* health_get(entity_id id);

#endif