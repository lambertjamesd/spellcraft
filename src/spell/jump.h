#ifndef __SPELL_JUMP_H__
#define __SPELL_JUMP_H__

#include "spell_sources.h"
#include "mana_regulator.h"
#include "spell_event.h"
#include "../effects/dash_trail.h"
#include "mana_regulator.h"
#include "elements.h"

struct jump {
    struct spell_data_source* data_source;
    struct mana_regulator mana_regulator;

    bool did_start;
};

void jump_init(struct jump* jump, struct spell_data_source* source, struct spell_event_options event_options);
void jump_destroy(struct jump* jump);

bool jump_update(struct jump* jump, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif