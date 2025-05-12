#ifndef __SPELL_SHEILD_H__
#define __SPELL_SHEILD_H__

#include "../collision/dynamic_object.h"
#include "../effects/mesh_animation.h"
#include "../entity/health.h"
#include "../math/transform.h"
#include "../entity/health_shield.h"
#include "spell_event.h"
#include "spell_sources.h"

struct shield_flag {
    uint16_t did_parry: 1;
};


struct shield {
    struct Transform transform;
    struct spell_data_source* data_source;
    struct mesh_animation* start_animation;
    struct dynamic_object dynamic_object;
    struct health health;
    struct health_shield shield;
    enum element_type element;
    float parry_timer;
    float hold_radius;
    float lifetime;
    struct shield_flag flags;
};

void shield_init(struct shield* shield, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element);
void shield_destroy(struct shield* shield);
bool shield_update(struct shield* shield, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif