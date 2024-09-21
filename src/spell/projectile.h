#ifndef __SPELL_PROJECTILE_H__
#define __SPELL_PROJECTILE_H__

#include "../math/vector3.h"
#include "../collision/dynamic_object.h"
#include "../effects/mesh_animation.h"

#include "spell_sources.h"
#include "spell_event.h"
#include "elements.h"

struct projectile {
    struct Vector3 pos;
    struct spell_data_source* data_source;
    struct spell_data_source* data_output;
    struct dynamic_object dynamic_object;
    struct mesh_animation* start_animation;
    uint16_t has_hit: 1;
    uint16_t has_primary_event: 1;
    uint16_t has_secondary_event: 1;
    uint16_t is_controlled: 1;
    uint16_t element: 2;
};

void projectile_init(struct projectile* projectile, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element);
void projectile_destroy(struct projectile* projectile);

void projectile_update(struct projectile* projectile, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif