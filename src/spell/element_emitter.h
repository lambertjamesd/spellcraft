#ifndef __SPELL_ELEMENT_EMITTER_H__
#define __SPELL_ELEMENT_EMITTER_H__

#include <stdbool.h>
#include "spell_sources.h"
#include "spell_event.h"
#include "../math/transform_single_axis.h"
#include "../effects/scale_in_fade_out.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"
#include "elements.h"

typedef void* (*on_effect_start)(struct Vector3* position, struct Vector3* direction, float scale);
typedef void* (*on_effect_update)(void* effect, struct Vector3* position, struct Vector3* direction, float scale);
typedef void (*on_effect_stop)(void* effect);
typedef bool (*is_effect_running)(void* effect);
typedef bool (*effect_free)(void* effect);

struct element_emitter_definition {
    enum element_type element_type;
    struct dynamic_object_type collider_type;
    float scale;
    float mana_per_second;
    float damage_per_frame;
    on_effect_start on_effect_start;
    on_effect_update on_effect_update;
    on_effect_stop on_effect_stop;
    is_effect_running is_effect_running;
    effect_free effect_free;
};

struct element_emitter {
    struct spell_data_source* data_source;
    struct dynamic_object dynamic_object;
    struct TransformSingleAxis transform;
    struct element_emitter_definition* effect_definition;
    void* effect;
    bool is_active;
};

extern struct element_emitter_definition fire_definition;
extern struct element_emitter_definition fire_around_definition;
extern struct element_emitter_definition ice_definition;
extern struct element_emitter_definition lightning_definition;
extern struct element_emitter_definition lightning_around_definition;

void element_emitter_init(struct element_emitter* element_emitter, struct spell_data_source* source, struct spell_event_options event_options, struct element_emitter_definition* effect_definition);
void element_emitter_destroy(struct element_emitter* element_emitter);

void element_emitter_update(struct element_emitter* element_emitter, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif