#ifndef __SPELL_WIND_H__
#define __SPELL_WIND_H__

#include "../math/transform_single_axis.h"
#include "spell_sources.h"
#include "spell_event.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "elements.h"

enum wind_flags {
    WIND_FLAGS_LIGHTNING = 1 << 0,
    WIND_FLAGS_ICY       = 1 << 1,

    WIND_FLAGS_DID_BURST = 1 << 2,
};

struct wind_definition {
    float acceleration;
    float top_speed;
    float burst_time;
    uint16_t flags;
};

#define MAX_WIND_BONES   3

#define MAX_PUSHING_ENTITIES    8

struct wind {
    struct spell_data_source* data_source;
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object dynamic_object;
    struct wind_definition* definition;

    struct Quaternion bone_rotations[MAX_WIND_BONES];

    float push_timer;
    uint16_t flags;

    uint8_t current_pushing_count;

    entity_id pushing_entities[MAX_PUSHING_ENTITIES];
};

void wind_init(struct wind* wind, struct spell_data_source* source, struct spell_event_options event_options, struct wind_definition* effect_definition);
void wind_destroy(struct wind* wind);

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

struct wind_definition* wind_lookup_definition(enum element_type element);

#endif