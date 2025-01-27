#include "recast.h"

void recast_init(struct recast* recast, struct spell_data_source* source, struct spell_event_options event_options, enum recast_mode mode) {
    recast->original_source = source;
    spell_data_source_retain(recast->original_source);
    recast->recast_source = 0;
    recast->output = 0;
    recast->next_recast = 0;
    recast->mode = mode;
    recast->burst_mana = 0.0f;
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

void recast_recast(struct recast* recast, struct spell_data_source* recast_source, float burst_mana) {
    recast->recast_source = recast_source;
    recast->burst_mana = burst_mana;
    spell_data_source_retain(recast->recast_source);
}

bool recast_update_end_cast(struct recast* recast, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    if (!recast->output) {
        struct spell_data_source* output = spell_data_source_pool_get(pool);
        
        if (!output) {
            return false;
        }

        spell_data_source_retain(output);
        recast->output = output;

        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, output, recast->burst_mana);
    }

    recast->output->position = recast->original_source->position;
    recast->output->direction = recast->original_source->direction;
    recast->output->flags = recast->original_source->flags;
    recast->output->flags.cast_state = SPELL_CAST_STATE_ACTIVE;
    recast->output->target = recast->original_source->target;
    
    if (recast->recast_source) {
        recast->output->flags.cast_state = SPELL_CAST_STATE_INACTIVE;
        return false;
    }

    return true;
}

bool recast_update(struct recast* recast, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (recast->mode == REACT_MODE_STICKY) {
        return recast_update_end_cast(recast, event_listener, &spell_sources->data_sources);
    }

    if (recast->recast_source && !recast->output) {
        struct spell_data_source* output = spell_data_source_pool_get(&spell_sources->data_sources);

        if (!output) {
            return false;
        }

        spell_data_source_retain(output);
        recast->output = output;

        output->position = recast->original_source->position;
        output->flags = recast->original_source->flags;
        if (recast->recast_source->flags.windy) {
            output->direction = recast->recast_source->direction;
        } else {
            output->direction = recast->original_source->direction;
        }
        output->flags.cast_state = recast->recast_source->flags.cast_state;
        output->target = recast->original_source->target;

        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, output, recast->burst_mana);
    }

    if (recast->output) {
        recast->output->position = recast->original_source->position;
        recast->output->flags = recast->original_source->flags;
        if (recast->recast_source->flags.windy) {
            recast->output->direction = recast->recast_source->direction;
        } else {
            recast->output->direction = recast->original_source->direction;
        }
        recast->output->flags.cast_state = recast->recast_source->flags.cast_state;

        if (recast->recast_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
            return false;
        }
    }

    return true;
}