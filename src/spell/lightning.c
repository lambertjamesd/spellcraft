#include "lightning.h"

static struct lightning_effect_def lightning_def = {
    .spread = 1.1f,
};


void lightning_init(struct lightning* lightning, struct spell_data_source* source) {
    lightning->effect = lightning_effect_new(&source->position, &lightning_def);
    lightning->data_source = source;
    spell_data_source_retain(source);
}

void lightning_destroy(struct lightning* lightning) {
    spell_data_source_release(lightning->data_source);
    lightning_effect_free(lightning->effect);
}

void lightning_update(struct lightning* lightning, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    lightning_effect_set_position(lightning->effect, &lightning->data_source->position, &lightning->data_source->direction);

    if (lightning->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }
}