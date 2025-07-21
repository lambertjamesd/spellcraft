#ifndef __SPELL_ELEMENTAL_SWORD_H__
#define __SPELL_ELEMENTAL_SWORD_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../render/tmesh.h"
#include "../effects/sword_trail.h"
#include "../collision/dynamic_object.h"
#include "../entity/damage.h"

#include "../math/transform_single_axis.h"

struct elemental_sword {
    struct spell_data_source* data_source;
    struct tmesh* mesh;
    struct sword_trail* trail;
    struct dynamic_object collider;
    struct dynamic_object_type collider_type;
    struct swing_shape swing_shape;
    struct damaged_set damaged_set;
};

void elemental_sword_init(struct elemental_sword* elemental_sword, struct spell_data_source* source, struct spell_event_options event_options);
void elemental_sword_destroy(struct elemental_sword* elemental_sword);

bool elemental_sword_update(struct elemental_sword* elemental_sword, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif