#ifndef __ENTITIES_PULLEY_GATE_H__
#define __ENTITIES_PULLEY_GATE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

struct pulley_gate {
    transform_sa_t gate_transform;
    vector3_t gate_position;
    vector3_t harness_position;
    renderable_t renderable;
    dynamic_object_t gate_collider;
    dynamic_object_t harness_collider;

    boolean_variable output;

    float gate_start_y;
    float harness_start_y;

    float velocity;
};

typedef struct pulley_gate pulley_gate_t;

void pulley_gate_init(pulley_gate_t* pulley_gate, struct pulley_gate_definition* definition, entity_id entity_id);
void pulley_gate_destroy(pulley_gate_t* pulley_gate, struct pulley_gate_definition* definition);
void pulley_gate_common_init();
void pulley_gate_common_destroy();

#endif