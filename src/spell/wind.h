#ifndef __SPELL_WIND_H__
#define __SPELL_WIND_H__

#include "../math/transform_single_axis.h"
#include "spell_sources.h"
#include "spell_event.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"

struct wind_definition {
    float scale;
};

#define MAX_WIND_BONES   3

struct wind {
    struct spell_data_source* data_source;
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object dynamic_object;

    struct Quaternion bone_rotations[MAX_WIND_BONES];
};

void wind_init(struct wind* wind, struct spell_data_source* source, struct spell_event_options event_options, struct wind_definition* effect_definition);
void wind_destroy(struct wind* wind);

bool wind_update(struct wind* wind, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif