#ifndef __PUZZLE_ELEVATOR_H__
#define __PUZZLE_ELEVATOR_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../collision/dynamic_object.h"
#include "../render/renderable.h"

struct elevator {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct dynamic_object collision;
    struct Vector3 start_position;
    struct Vector3 end_position;
    float timer;
    boolean_variable enabled;
    bool inv_enabled;
    bool is_returning;
};

void elevator_init(struct elevator* elevator, struct elevator_definition* definition);
void elevator_destroy(struct elevator* elevator);

#endif