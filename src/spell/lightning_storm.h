#ifndef __SPELL_LIGHTNING_STORM_H__
#define __SPELL_LIGHTNING_STORM_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../math/vector3.h"
#include "../collision/spatial_trigger.h"
#include "./lightning_strike.h"

#define LIGHTNING_STORM_ACTIVE_STRIKE_COUNT  5

#define LIGHTNING_STORM_MAX_STRIKE_COUNT                     16

struct lightning_storm {
    transform_sa_t transform;
    struct spell_data_source* data_source;
    spatial_trigger_t trigger;
    lightning_strike_t strikes[LIGHTNING_STORM_ACTIVE_STRIKE_COUNT];
    uint8_t first_active_strike;
    uint8_t next_target_strike;
    uint8_t active_strike_count;
    uint8_t total_target_count;
    float strike_timer;
    entity_id targets[LIGHTNING_STORM_MAX_STRIKE_COUNT];
};

typedef struct lightning_storm lightning_storm_t;

void lightning_storm_init(lightning_storm_t* storm, struct spell_data_source* source, struct spell_event_options event_options);
void lightning_storm_destroy(lightning_storm_t* storm);

bool lightning_storm_update(lightning_storm_t* storm);

#endif