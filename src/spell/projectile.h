#ifndef __SPELL_PROJECTILE_H__
#define __SPELL_PROJECTILE_H__

#include "../math/vector3.h"
#include "../collision/dynamic_object.h"

#include "spell_data_source.h"
#include "spell_event.h"

struct projectile {
    struct Vector3 pos;
    struct spell_data_source* data_source;
    struct spell_data_source* data_output;
    struct dynamic_object dynamic_object;
    uint16_t has_hit: 1;
    uint16_t has_primary_event: 1;
    uint16_t has_secondary_event: 1;
    uint16_t is_controlled: 1;
    int render_id;
};

void projectile_init(struct projectile* projectile, struct spell_data_source* source, struct spell_event_options event_options);
void projectile_destroy(struct projectile* projectile);

void projectile_update(struct projectile* projectile, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);

#endif