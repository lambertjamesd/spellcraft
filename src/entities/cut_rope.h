#ifndef __ENTITIES_CUT_ROPE_H__
#define __ENTITIES_CUT_ROPE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../collision/shapes/swing_shape.h"
#include "entity_deps.h"

struct cut_rope {
    vector3_t position;
    dynamic_object_type_t collider_type;
    swing_shape_t swing_shape;
    dynamic_object_t collider;
    health_t health;
    vector3_t last_tip;
    vector3_t tip_velocity;
    entity_spawner connected_to;
    float length;
};

typedef struct cut_rope cut_rope_t;

void cut_rope_init(cut_rope_t* cut_rope, struct cut_rope_definition* definition, entity_id entity_id);
void cut_rope_destroy(cut_rope_t* cut_rope, struct cut_rope_definition* definition);
void cut_rope_common_init();
void cut_rope_common_destroy();

#endif