#include "./heal.h"


void spell_heal_init(struct spell_heal* heal, struct spell_data_source* source, struct spell_event_options event_options) {
    heal->data_source = source;
    spell_data_source_retain(source);
}

void spell_heal_destroy(struct spell_heal* heal) {
    spell_data_source_release(heal->data_source);
}

bool spell_heal_update(struct spell_heal* heal, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    return heal->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE;
}