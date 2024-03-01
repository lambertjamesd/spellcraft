#ifndef __SPELL_FIRE_H__
#define __SPELL_FIRE_H__

#include "spell_data_source.h"
#include "spell_event.h"

#define MAX_FIRE_PARTICLE_COUNT     8

struct fire {
    struct spell_data_source* data_source;
    struct Vector3 particle_offset[MAX_FIRE_PARTICLE_COUNT];
    int render_id;
    float cycle_time;
    float total_time;
    float end_time;
    uint16_t index_offset;
};

void fire_init(struct fire* fire, struct spell_data_source* source, struct spell_event_options event_options);
void fire_destroy(struct fire* fire);

void fire_update(struct fire* fire, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool);

#endif