#ifndef __SPELL_WIND_H__
#define __SPELL_WIND_H__

#include "../math/transform_single_axis.h"
#include "spell_sources.h"
#include "spell_event.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "elements.h"
#include "push_single_target.h"

struct wind_definition {
    struct push_single_definition push;
    uint8_t sphere: 1;
    uint8_t icy: 1;
    uint8_t lightning: 1;
    float base_scale;
};

#define MAX_WIND_BONES   3

struct wind {
    struct spell_data_source* data_source;
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object dynamic_object;
    struct wind_definition* definition;

    struct Quaternion bone_rotations[MAX_WIND_BONES];

    float push_timer;

    uint8_t did_burst: 1;
};

void wind_init(struct wind* wind, struct spell_data_source* source, struct spell_event_options event_options, struct wind_definition* effect_definition);
void wind_destroy(struct wind* wind);

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

struct wind_definition* wind_lookup_definition(enum element_type element, bool has_ground);

#endif