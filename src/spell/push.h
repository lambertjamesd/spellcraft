#ifndef __SPELL_PUSH_H__
#define __SPELL_PUSH_H__

#include "spell_sources.h"
#include "mana_regulator.h"
#include "spell_event.h"
#include "../effects/dash_trail.h"
#include "mana_regulator.h"
#include "elements.h"

struct push {
    struct spell_data_source* data_source;
    struct mana_regulator mana_regulator;
    struct dash_trail* dash_trail_left;
    struct dash_trail* dash_trail_right;
    uint8_t push_mode;
    uint8_t should_restore_tangible;
};

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options, enum element_type push_mode);
void push_destroy(struct push* push);

bool push_update(struct push* push, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif