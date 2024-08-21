#ifndef __SPELL_FIRE_H__
#define __SPELL_FIRE_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"
#include "elements.h"

#define MAX_FIRE_PARTICLE_COUNT     8

struct fire {
    struct spell_data_source* data_source;
    struct Vector3 particle_offset[MAX_FIRE_PARTICLE_COUNT];
    struct dynamic_object dynamic_object;
    struct Vector3 position;
    struct Vector2 rotation;
    float cycle_time;
    float total_time;
    float end_time;
    uint16_t index_offset;
    uint8_t element_type;
};

void fire_init(struct fire* fire, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type);
void fire_destroy(struct fire* fire);

void fire_update(struct fire* fire, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

void fire_apply_damage(struct dynamic_object* dyanmic_object, enum damage_type damage_type);
enum damage_type fire_determine_damage_type(enum element_type element_type);

#endif