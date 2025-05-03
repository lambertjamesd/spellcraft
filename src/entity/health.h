#ifndef __ENTITY_HEALTH_H__
#define __ENTITY_HEALTH_H__

#include "entity_id.h"
#include <stdint.h>
#include <stdbool.h>
#include "../spell/elements.h"
#include "../math/vector3.h"
#include "damage.h"
#include "health_shield.h"

typedef void (*health_damage_callback)(void* data, struct damage_info* damage);

struct health {
    entity_id entity_id;
    float max_health;
    float current_health;
    float status_timer;
    enum damage_type current_status;

    health_damage_callback callback;
    void* callback_data;

    struct health_shield* health_shield;
};

struct dynamic_object;

void health_reset();

void health_init(struct health* health, entity_id id, float max_health);
void health_set_callback(struct health* health, health_damage_callback callback, void* data);
void health_destroy(struct health* health);

void health_damage(struct health* health, struct damage_info* damage);
void health_damage_id(entity_id target, struct damage_info* damage);

void health_apply_contact_damage(struct dynamic_object* damage_source, float amount, enum damage_type type);

struct health* health_get(entity_id id);

bool health_is_burning(struct health* health);
bool health_is_frozen(struct health* health);
bool health_is_shocked(struct health* health);
bool health_is_alive(struct health* health);

void health_add_shield(struct health* health, struct health_shield* health_shield);
void health_remove_shield(struct health* health, struct health_shield* health_shield);

enum damage_type health_determine_damage_type(enum element_type element_type);

#endif