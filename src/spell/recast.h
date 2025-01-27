#ifndef __SPELL_RECAST_H__
#define __SPELL_RECAST_H__

#include "spell_sources.h"
#include "spell_event.h"
#include <stdbool.h>

enum recast_mode {
    RECAST_MODE_RECAST,
    REACT_MODE_STICKY,
};

struct recast {
    struct recast* next_recast;
    struct spell_data_source* recast_source;
    struct spell_data_source* original_source;
    struct spell_data_source* output;
    enum recast_mode mode;
    float burst_mana;
};

void recast_init(struct recast* recast, struct spell_data_source* source, struct spell_event_options event_options, enum recast_mode mode);
void recast_destroy(struct recast* recast);

void recast_recast(struct recast* recast, struct spell_data_source* recast_source, float burst_mana);
bool recast_update(struct recast* recast, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif