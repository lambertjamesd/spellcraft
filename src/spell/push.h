#ifndef __SPELL_PUSH_H__
#define __SPELL_PUSH_H__

#include "spell_data_source.h"
#include "spell_event.h"

struct push {
    struct spell_data_source* data_source;
};

void push_init(struct push* push, struct spell_data_source* source, struct spell_event_options event_options);
void push_destroy(struct push* push);

void push_update(struct push* push, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);


#endif