#include "spell_event.h"

void spell_event_listener_init(struct spell_event_listener* listener) {
    listener->event_count = 0;
}

void spell_event_listener_add(struct spell_event_listener* listener, enum spell_event_type type, struct spell_data_source* data_source) {
    if (listener->event_count == MAX_EVENT_COUNT) {
        if (type == SPELL_EVENT_DESTROY) {
            // destroy events have priority
            listener->events[MAX_EVENT_COUNT - 1].data_source = data_source;
            listener->events[MAX_EVENT_COUNT - 1].type = SPELL_EVENT_DESTROY;
        }
        return;
    }

    listener->events[listener->event_count].type = type;
    listener->events[listener->event_count].data_source = data_source;
    listener->event_count += 1;
}