#ifndef __SPELL_FIRE_AROUND_H__
#define __SPELL_FIRE_AROUND_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../collision/dynamic_object.h"
#include "elements.h"

struct fire_around {
    struct spell_data_source* data_source;
    struct dynamic_object dynamic_object;
    struct Vector3 position;
    float timer;
    float end_time;
    uint8_t element_type;
};


void fire_around_init(struct fire_around* fire_around, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type);
void fire_around_destroy(struct fire_around* fire_around);

void fire_around_update(struct fire_around* fire_around, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif