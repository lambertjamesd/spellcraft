#ifndef __SPELL_EXPLOSION_H__
#define __SPELL_EXPLOSION_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "elements.h"
#include <stdbool.h>

struct explosion {
    struct spell_data_source* data_source;
    struct Vector3 position;
    float total_time;
    uint8_t element_type;
};

void explosion_init(struct explosion* explosion, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type);
void explosion_destroy(struct explosion* explosion);

bool explosion_update(struct explosion* explosion, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);


#endif