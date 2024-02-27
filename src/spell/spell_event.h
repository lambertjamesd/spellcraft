#ifndef __SPELL_SPELL_EVENT_H__
#define __SPELL_SPELL_EVENT_H__

#include <stdint.h>

#define MAX_EVENT_COUNT 4

enum spell_event_type {
    SPELL_EVENT_PRIMARY,
    SPELL_EVENT_SECONDARY,
    SPELL_EVENT_DESTROY,
};

struct spell_event_listener {
    uint8_t events[MAX_EVENT_COUNT];
    uint8_t event_count;
};


void spell_event_listener_init(struct spell_event_listener* listener);
void spell_event_listener_add(struct spell_event_listener* listener, enum spell_event_type type);

#endif