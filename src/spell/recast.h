#ifndef __SPELL_RECAST_H__
#define __SPELL_RECAST_H__

#include "spell_data_source.h"
#include "spell_event.h"

struct recast {
    struct recast* next_recast;
    struct spell_data_source* recast_source;
    struct spell_data_source* original_source;
    struct spell_data_source* output;
};

void recast_init(struct recast* recast, struct spell_data_source* source, struct spell_event_options event_options);
void recast_destroy(struct recast* recast);

void recast_recast(struct recast* recast, struct spell_data_source* recast_source);
void recast_update(struct recast* recast, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);

#endif