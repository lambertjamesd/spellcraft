#ifndef __ENTITY_HEALTH_H__
#define __ENTITY_HEALTH_H__

#include "entity_id.h"
#include <stdint.h>
#include <stdbool.h>
#include "../spell/elements.h"

enum damage_type {
    DAMAGE_TYPE_PROJECTILE = (1 << 0),
    DAMAGE_TYPE_FIRE = (1 << 1),
    DAMAGE_TYPE_ICE = (1 << 2),
    DAMAGE_TYPE_LIGHTING = (1 << 3),
    DAMAGE_TYPE_WATER = (1 << 4),
    DAMAGE_TYPE_BASH = (1 << 5),
};

typedef void (*health_damage_callback)(void* data, float amount, entity_id source, enum damage_type type);

struct health {
    entity_id entity_id;
    float max_health;
    float current_health;
    float status_timer;
    enum damage_type current_status;

    health_damage_callback callback;
    void* callback_data;
};

struct dynamic_object;

void health_reset();

void health_init(struct health* health, entity_id id, float max_health);
void health_set_callback(struct health* health, health_damage_callback callback, void* data);
void health_destroy(struct health* health);

void health_damage(struct health* health, float amount, entity_id source, enum damage_type type);
void health_damage_id(entity_id target, float amount, entity_id source, enum damage_type type);

void health_apply_contact_damage(struct dynamic_object* damage_source, float amount, enum damage_type type);

struct health* health_get(entity_id id);

bool health_is_burning(struct health* health);
bool health_is_frozen(struct health* health);
bool health_is_shocked(struct health* health);
bool health_is_alive(struct health* health);

enum damage_type health_determine_damage_type(enum element_type element_type);

#endif