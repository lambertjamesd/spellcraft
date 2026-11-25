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
    SPELL_ANIMATION_SWING_0,
    SPELL_ANIMATION_SWING_1,
    SPELL_ANIMATION_SWING_2,
    SPELL_ANIMATION_SPIN,
    SPELL_ANIMATION_CAST_UP,
    SPELL_ANIMATION_CAST_FORWARD,
    SPELL_ANIMATION_CAST_FORWARD_HOLD,
    SPELL_ANIMATION_CAST_GROUND,

    SPELL_ANIMATION_COUNT,
};

typedef enum spell_animation spell_animation_t;

enum spell_cast_source {
    SPELL_CAST_SOURCE_CENTER,
    SPELL_CAST_SOURCE_STAFF,
    SPELL_CAST_SOURCE_FEET,
    SPELL_CAST_SOURCE_STAFF_TIP,
};

typedef enum spell_cast_source spell_cast_source_t;

union spell_source_flags {
    struct {
        uint16_t cast_state: 2;
        uint16_t has_animator: 1;
        uint16_t is_animating: 1;
        uint16_t can_recast: 1;
        uint16_t cast_held: 1;
        uint16_t animator_attack: 1;
        uint16_t skip_blending: 1;
    };
    uint16_t all;
};

struct spell_data_source {
    vector3_t position;
    vector3_t direction;
    vector3_t staff_pos;
    vector3_t staff_dir;
    float half_height;
    union spell_source_flags flags;
    uint8_t reference_count;
    uint8_t request_animation;
    entity_id target;
};

typedef struct spell_data_source spell_data_source_t;

struct spell_event_options {
    uint16_t has_next_rune: 1;
    rune_pattern_t rune;
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
bool spell_data_source_request_animation(struct spell_data_source* data_source, enum spell_animation animation, bool skip_blending);
void spell_data_source_cancel_animation(struct spell_data_source* data_source);

static inline void spell_data_source_get_pos(spell_data_source_t* data_source, spell_cast_source_t source, vector3_t* position) {
    switch (source) {
        case SPELL_CAST_SOURCE_CENTER:
            *position = data_source->position;
            position->y += data_source->half_height;
            break;
        case SPELL_CAST_SOURCE_STAFF:
            *position = data_source->staff_pos;
            break;
        case SPELL_CAST_SOURCE_STAFF_TIP:
            vector3AddScaled(&data_source->staff_pos, &data_source->staff_dir, 1.0f, position);
            break;
        default:
            *position = data_source->position;
            break;
    }
}

static inline void spell_data_source_get_dir(spell_data_source_t* data_source, spell_cast_source_t source, vector3_t* position) {
    switch (source) {
        case SPELL_CAST_SOURCE_STAFF:
        case SPELL_CAST_SOURCE_STAFF_TIP:
            *position = data_source->staff_dir;
            break;
        default:
            *position = data_source->direction;
            break;
    }
}

void spell_data_source_apply_transform_sa(struct spell_data_source* data_source, spell_cast_source_t source, struct TransformSingleAxis* transform);

enum element_type spell_data_source_determine_element(rune_pattern_t rune);

#endif