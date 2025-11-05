#ifndef __PUZZLE_POTTERY_WHEEL_H__
#define __PUZZLE_POTTERY_WHEEL_H__

#include <stdint.h>
#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"
#include "../render/renderable.h"

#define MAX_WHEEL_ORIENTATIONS  4

struct pottery_wheel {
    transform_sa_t transform;
    renderable_t renderable;
    uint8_t target_orientation;
    bool last_input_value;
    boolean_variable input;
    integer_variable output;

    struct Vector2 max_rotation;
};

typedef struct pottery_wheel pottery_wheel_t;

void pottery_wheel_init(pottery_wheel_t* wheel, struct pottery_wheel_definition* definition, entity_id entity_id);
void pottery_wheel_destroy(pottery_wheel_t* wheel);

#endif