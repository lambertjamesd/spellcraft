#ifndef __SPELL_HEAL_H__
#define __SPELL_HEAL_H__

#include "spell_sources.h"
#include "mana_regulator.h"
#include "spell_event.h"

struct heal_flags {
    uint16_t instant: 1;
    uint16_t reverse: 1;
    uint16_t aoe: 1;
};

struct heal_definition {
    float rate;
    float effeciency;
    struct heal_flags flags;
};

struct spell_heal {
    struct spell_data_source* data_source;
    struct mana_regulator mana_regulator;

    struct heal_definition* definition;

    struct heal_flags flags;
    float mana_stored;
};

void spell_heal_init(struct spell_heal* heal, struct spell_data_source* source, struct spell_event_options event_options);
void spell_heal_destroy(struct spell_heal* heal);

bool spell_heal_update(struct spell_heal* heal, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif