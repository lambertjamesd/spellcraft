#ifndef _SPELL_SPRITE_H__
#define _SPELL_SPRITE_H__

#include "../entity/entity_id.h"
#include "../math/transform_single_axis.h"
#include "../collision/dynamic_object.h"
#include "spell_event.h"
#include "spell_data_source.h"
#include "spell_sources.h"
#include "../entity/health.h"
#include "elements.h"
#include "../render/renderable.h"
#include <stdbool.h>

struct living_sprite;
struct render_batch;

typedef void (*sprite_on_contact)(struct living_sprite* sprite, struct contact* contact, float portion);

struct living_sprite_definition {
    enum element_type element_type;
    struct dynamic_object_type collider_type;
    float damage;
    const char* model_file;
    sprite_on_contact on_contact;
};

struct living_sprite_flags {
    uint16_t is_attacking: 1;
    uint16_t is_mine: 1;
    uint16_t did_apply_aeo;
};

struct living_sprite {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object collider;
    struct dynamic_object vision;
    entity_id target;
    struct health health;
    struct living_sprite_flags flags;
    float explode_timer;
    struct living_sprite_definition* definition;
};

void living_sprite_init(struct living_sprite* living_sprite, struct spell_data_source* source, struct spell_event_options event_options, struct living_sprite_definition* definition);
bool living_sprite_update(struct living_sprite* living_sprite, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);
void living_sprite_destroy(struct living_sprite* living_sprite);

#endif