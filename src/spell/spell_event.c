#include "spell_event.h"

void spell_event_listener_init(struct spell_event_listener* listener) {
    listener->event_count = 0;
}

void spell_event_listener_add(struct spell_event_listener* listener, enum spell_event_type type, struct spell_data_source* data_source, float burst_mana) {
    spell_data_source_retain(data_source);

    if (listener->event_count == MAX_EVENT_COUNT) {
        return;
    }

    listener->events[listener->event_count].type = type;
    listener->events[listener->event_count].data_source = data_source;
    listener->events[listener->event_count].burst_mana = burst_mana;
    listener->event_count += 1;
}   

void spell_event_listener_destroy(struct spell_event_listener* listener) {
    for (int i = 0; i < listener->event_count; ++i) {
        spell_data_source_release(listener->events[i].data_source);
    }
}