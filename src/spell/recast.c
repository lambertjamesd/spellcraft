#include "recast.h"

void recast_init(struct recast* recast, struct spell_data_source* source, struct spell_event_options event_options) {
    recast->original_source = source;
    spell_data_source_retain(recast->original_source);
    recast->recast_source = 0;
    recast->output = 0;
    recast->next_recast = 0;
}

void recast_destroy(struct recast* recast) {
    spell_data_source_release(recast->original_source);

    if (recast->recast_source) {
        spell_data_source_release(recast->recast_source);
    }
    
    if (recast->output) {
        spell_data_source_release(recast->output);
    }
}

void recast_recast(struct recast* recast, struct spell_data_source* recast_source) {
    recast->recast_source = recast_source;
    spell_data_source_retain(recast->recast_source);
}

void recast_update(struct recast* recast, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    if (recast->recast_source && !recast->output) {
        struct spell_data_source* output = spell_data_source_pool_get(pool);

        if (!output) {
            spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
            return;
        }

        spell_data_source_retain(output);
        recast->output = output;

        output->position = recast->original_source->position;
        output->flags = recast->original_source->flags;
        output->direction = recast->recast_source->direction;
        output->flags.cast_state = recast->recast_source->flags.cast_state;
        output->target = recast->recast_source->target;

        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, output);
    }

    if (recast->output) {
        recast->output->position = recast->original_source->position;
        recast->output->flags = recast->original_source->flags;
        recast->output->direction = recast->recast_source->direction;
        recast->output->flags.cast_state = recast->recast_source->flags.cast_state;

        if (recast->recast_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
            spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
        }
    }
}