#ifndef __SPELL_ELEMENTAL_SWORD_H__
#define __SPELL_ELEMENTAL_SWORD_H__

#include "spell_sources.h"
#include "spell_event.h"
#include "../render/tmesh.h"
#include "../effects/sword_trail.h"
#include "../collision/dynamic_object.h"
#include "../entity/damage.h"

#include "../math/transform_single_axis.h"

struct elemental_sword_definition {
    enum spell_animation animation;
    struct damage_source damage_source;
    float sword_length;
    float mana_cost;
    float free_swing_time;
    float free_swing_angle;
    float free_swing_velocity;
    color_t color;
};

struct elemental_sword {
    struct spell_data_source* data_source;
    struct tmesh* mesh;
    struct sword_trail* trail;
    struct dynamic_object collider;
    struct dynamic_object_type collider_type;
    struct swing_shape swing_shape;
    struct damaged_set damaged_set;

    struct elemental_sword_definition* definition;

    uint16_t needs_mana_check: 1;
    uint16_t has_animation: 1;
    
    float power_ratio;
    float animation_time;
};

void elemental_sword_init(struct elemental_sword* elemental_sword, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type);
void elemental_sword_destroy(struct elemental_sword* elemental_sword);

bool elemental_sword_update(struct elemental_sword* elemental_sword, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif