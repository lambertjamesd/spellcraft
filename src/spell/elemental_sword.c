#include "elemental_sword.h"



void elemental_sword_init(struct elemental_sword* elemental_sword, struct spell_data_source* source, struct spell_event_options event_options) {
    elemental_sword->data_source = spell_data_source_retain(source);

    spell_data_source_request_animation(source, SPELL_ANIMATION_SWING);
}

void elemental_sword_destroy(struct elemental_sword* elemental_sword) {
    spell_data_source_release(elemental_sword->data_source);
}

bool elemental_sword_update(struct elemental_sword* elemental_sword, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    return elemental_sword->data_source->flags.is_animating;
}