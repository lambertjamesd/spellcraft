#include "spell_event.h"

void spell_event_listener_init(struct spell_event_listener* listener) {
    listener->event_count = 0;
}

void spell_event_listener_add(struct spell_event_listener* listener, enum spell_event_type type, struct spell_data_source* data_source) {
    spell_data_source_retain(data_source);

    if (listener->event_count == MAX_EVENT_COUNT) {
        struct spell_event* last_event = &listener->events[MAX_EVENT_COUNT - 1];
        
        if (type == SPELL_EVENT_DESTROY) {
            spell_data_source_release(last_event->data_source);

            // destroy events have priority
            last_event->data_source = data_source;
            last_event->type = SPELL_EVENT_DESTROY;
        }
        return;
    }

    listener->events[listener->event_count].type = type;
    listener->events[listener->event_count].data_source = data_source;
    listener->event_count += 1;
}   

void spell_event_listener_destroy(struct spell_event_listener* listener) {
    for (int i = 0; i < listener->event_count; ++i) {
        spell_data_source_release(listener->events[i].data_source);
    }
}