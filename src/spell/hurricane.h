#ifndef __SPELL_HURRICANE_H__
#define __SPELL_HURRICANE_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/spatial_trigger.h"

#include "spell_sources.h"
#include "spell_event.h"

struct hurricane {
    transform_sa_t transform;
    renderable_t renderable;
    spatial_trigger_t trigger;
    float timer;
    element_attr_t attrs[2];
};

typedef struct hurricane hurricane_t;

void hurricane_init(hurricane_t* hurricane, spell_data_source_t* source, spell_event_options_t event_options);
bool hurricane_update(hurricane_t* hurricane);
void hurricane_destroy(hurricane_t* hurricane);

#endif