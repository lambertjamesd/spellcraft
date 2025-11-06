#ifndef __PUZZLE_FAN_SWITCH_H__
#define __PUZZLE_FAN_SWITCH_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../entity/entity_id.h"

struct fan_switch {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    boolean_variable output;
};

typedef struct fan_switch fan_switch_t;

void fan_switch_init(fan_switch_t* fan_switch, struct fan_switch_definition* definition, entity_id entity_id);
void fan_switch_destroy(fan_switch_t* fan_switch);

#endif