#ifndef __SPELL_LIGHTNING_STORM_H__
#define __SPELL_LIGHTNING_STORM_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../math/vector3.h"
#include "../collision/spatial_trigger.h"
#include "./lightning_strike.h"

#define LIGHTNING_STORM_ACTIVE_STRIKE_COUNT  5

#define MAX_STRIKE_COUNT                     16

struct lightning_storm {
    transform_sa_t transform;
    struct spell_data_source* data_source;
    spatial_trigger_t trigger;
    lightning_strike_t strikes[LIGHTNING_STORM_ACTIVE_STRIKE_COUNT];
    uint8_t next_strike;
    uint8_t last_strike;
    uint8_t remaining_strike_count;
    uint8_t next_target_index;
    uint8_t total_target_count;
    entity_id targets[MAX_STRIKE_COUNT];
};

void lightning_storm_init(struct lightning_storm* storm, struct spell_data_source* source, struct spell_event_options event_options);
void lightning_storm_destroy(struct lightning_storm* storm);

bool lightning_storm_update(struct lightning_storm* storm);

#endif