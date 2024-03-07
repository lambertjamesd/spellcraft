#ifndef __SPELL_EXPLOSION_H__
#define __SPELL_EXPLOSION_H__

#include "spell_data_source.h"
#include "spell_event.h"

struct explosion {
    struct spell_data_source* data_source;
    struct Vector3 position;
    float total_time;
};

void explosion_init(struct explosion* explosion, struct spell_data_source* source, struct spell_event_options event_options);
void explosion_destroy(struct explosion* explosion);

void explosion_update(struct explosion* explosion, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);


#endif