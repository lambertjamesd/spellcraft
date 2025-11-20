#ifndef __PUZZLE_STEP_SWITCH_H__
#define __PUZZLE_STEP_SWITCH_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../collision/dynamic_object.h"
#include "../render/renderable.h"
#include "../entity/entity_id.h"

struct step_switch {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    float target_pos;
    boolean_variable output;
    bool last_state;
};

typedef struct step_switch step_switch_t;

void step_switch_init(step_switch_t* step_switch, struct step_switch_definition* definition, entity_id entity_id);
void step_switch_destroy(step_switch_t* step_switch);
void step_switch_common_init();
void step_switch_common_destroy();

#endif