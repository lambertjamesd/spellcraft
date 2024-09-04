#ifndef __SPELL_LIGHTNING_H__
#define __SPELL_LIGHTNING_H__

#include "../effects/lightning_effect.h"
#include "spell_sources.h"
#include "spell_event.h"

struct lightning {
    struct lightning_effect* effect;
    struct spell_data_source* data_source;
};

void lightning_init(struct lightning* lightning, struct spell_data_source* source);
void lightning_destroy(struct lightning* lightning);

void lightning_update(struct lightning* lightning, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif