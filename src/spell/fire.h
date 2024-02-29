#ifndef __SPELL_FIRE_H__
#define __SPELL_FIRE_H__

#include "spell_data_source.h"
#include "spell_event.h"

struct fire {
    struct spell_data_source* data_source;
    int render_id;
    float cycle_time;
    float total_time;
};

void fire_init(struct fire* fire, struct spell_data_source* source, struct spell_event_options event_options);
void fire_destroy(struct fire* fire);

void fire_update(struct fire* fire, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);

#endif