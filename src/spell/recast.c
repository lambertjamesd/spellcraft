#include "recast.h"

void recast_init(struct recast* recast, struct spell_data_source* source, struct spell_event_options event_options, enum recast_mode mode) {
    recast->original_source = source;
    spell_data_source_retain(recast->original_source);
    recast->recast_source = 0;
    recast->output = 0;
    recast->next_recast = 0;
    recast->mode = mode;
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

void recast_update_end_cast(struct recast* recast, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    if (!recast->output) {
        struct spell_data_source* output = spell_data_source_pool_get(pool);
        
        if (!output) {
            spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
            return;
        }

        spell_data_source_retain(output);
        recast->output = output;

        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, output);
    }

    recast->output->position = recast->original_source->position;
    recast->output->direction = recast->original_source->direction;
    recast->output->flags = recast->original_source->flags;
    // recast consumes this flag
    recast->output->flags.reversed = 0;
    recast->output->flags.cast_state = SPELL_CAST_STATE_ACTIVE;
    recast->output->target = recast->original_source->target;
    
    if (recast->recast_source) {
        recast->output->flags.cast_state = SPELL_CAST_STATE_INACTIVE;
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
    }
}

void recast_update(struct recast* recast, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    if (recast->mode == REACT_MODE_STICKY) {
        recast_update_end_cast(recast, event_listener, pool);
        return;
    }

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
        if (recast->recast_source->flags.controlled) {
            output->direction = recast->recast_source->direction;
        } else {
            output->direction = recast->original_source->direction;
        }
        output->flags.cast_state = recast->recast_source->flags.cast_state;
        output->target = recast->original_source->target;

        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, output);
    }

    if (recast->output) {
        recast->output->position = recast->original_source->position;
        recast->output->flags = recast->original_source->flags;
        if (recast->recast_source->flags.controlled) {
            recast->output->direction = recast->recast_source->direction;
        } else {
            recast->output->direction = recast->original_source->direction;
        }
        recast->output->flags.cast_state = recast->recast_source->flags.cast_state;

        if (recast->recast_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
            spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, 0);
        }
    }
}