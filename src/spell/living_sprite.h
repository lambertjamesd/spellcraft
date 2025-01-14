#ifndef _SPELL_SPRITE_H__
#define _SPELL_SPRITE_H__

#include "../entity/entity_id.h"
#include "../math/transform_single_axis.h"
#include "../collision/dynamic_object.h"
#include "spell_event.h"
#include "spell_data_source.h"
#include "spell_sources.h"

struct living_sprite {
    struct TransformSingleAxis transform;
    struct dynamic_object collider;
    entity_id target;
};

void living_sprite_init(struct living_sprite* living_sprite, struct spell_data_source* source, struct spell_event_options event_options);
void living_sprite_update(struct living_sprite* living_sprite, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);
void living_sprite_destroy(struct living_sprite* living_sprite);

#endif