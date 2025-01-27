#ifndef __SPELL_SPELL_DATA_SOURCE_H__
#define __SPELL_SPELL_DATA_SOURCE_H__

#include "../entity/entity_id.h"
#include "../math/vector3.h"
#include "../math/transform_single_axis.h"
#include "elements.h"
#include "spell.h"

#include <stdint.h>

#define MAX_SPELL_DATA_SOURCES  32

enum spell_cast_state {
    SPELL_CAST_STATE_INACTIVE,
    SPELL_CAST_STATE_ACTIVE,
    SPELL_CAST_STATE_INSTANT,
};

union spell_source_flags {
    struct {
        uint16_t cast_state: 2;
    };
    uint16_t all;
};

struct spell_data_source {
    struct Vector3 position;
    struct Vector3 direction;
    union spell_source_flags flags;
    entity_id target;

    uint8_t reference_count;
};

struct spell_event_options {
    uint32_t has_primary_event: 1;
    uint32_t has_secondary_event: 1;
    float burst_mana;
};

struct spell_data_source_pool {
    struct spell_data_source data_sources[MAX_SPELL_DATA_SOURCES];
    uint16_t next_data_source;
};

void spell_data_source_pool_init(struct spell_data_source_pool* pool);
struct spell_data_source* spell_data_source_pool_get(struct spell_data_source_pool* pool);

void spell_data_source_retain(struct spell_data_source* data_source);
void spell_data_source_release(struct spell_data_source* data_source);

void spell_data_source_apply_transform_sa(struct spell_data_source* data_source, struct TransformSingleAxis* transform);

enum element_type spell_data_source_determine_element(union spell_modifier_flags flags);

#endif