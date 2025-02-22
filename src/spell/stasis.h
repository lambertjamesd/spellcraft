#ifndef __SPELL_STASIS_H__
#define __SPELL_STASIS_H__

#include "spell_sources.h"
#include "mana_regulator.h"
#include "spell_event.h"
#include "../effects/dash_trail.h"
#include "mana_regulator.h"

struct stasis {
    struct spell_data_source* data_source;
    struct mana_regulator mana_regulator;
    struct Vector3 hover_location;
    struct Vector3 original_velocity;
};

void stasis_init(struct stasis* stasis, struct spell_data_source* source, struct spell_event_options event_options);
void stasis_destroy(struct stasis* stasis);

bool stasis_update(struct stasis* stasis, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);


#endif