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

enum spell_animation {
    SPELL_ANIMATION_NONE,
    SPELL_ANIMATION_SWING,
    SPELL_ANIMATION_SPIN,
};

union spell_source_flags {
    struct {
        uint16_t cast_state: 2;
        uint16_t has_animator: 1;
        uint16_t is_animating: 1;
    };
    uint16_t all;
};

struct spell_data_source {
    struct Vector3 position;
    struct Vector3 direction;
    union spell_source_flags flags;
    uint8_t reference_count;
    uint8_t request_animation;
    entity_id target;
};

typedef struct spell_data_source spell_data_source_t;

struct spell_event_options {
    uint16_t has_primary_event: 1;
    uint16_t has_secondary_event: 1;
    union spell_modifier_flags modifiers;
    float burst_mana;
};

typedef struct spell_event_options spell_event_options_t;

struct spell_data_source_pool {
    struct spell_data_source data_sources[MAX_SPELL_DATA_SOURCES];
    uint16_t next_data_source;
};

void spell_data_source_pool_init(struct spell_data_source_pool* pool);
struct spell_data_source* spell_data_source_pool_get(struct spell_data_source_pool* pool);

struct spell_data_source* spell_data_source_retain(struct spell_data_source* data_source);
void spell_data_source_release(struct spell_data_source* data_source);
bool spell_data_source_request_animation(struct spell_data_source* data_source, enum spell_animation animation);

void spell_data_source_apply_transform_sa(struct spell_data_source* data_source, struct TransformSingleAxis* transform);

enum element_type spell_data_source_determine_element(union spell_modifier_flags flags);

#endif