#ifndef __SPELL_LIGHTNING_H__
#define __SPELL_LIGHTNING_H__

#include "../effects/lightning_effect.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "spell_sources.h"
#include "spell_event.h"

struct lightning {
    struct lightning_effect* effect;
    struct spell_data_source* data_source;
    struct dynamic_object dynamic_object;
    struct TransformSingleAxis transform;
};

void lightning_init(struct lightning* lightning, struct spell_data_source* source);
void lightning_destroy(struct lightning* lightning);

void lightning_update(struct lightning* lightning, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif