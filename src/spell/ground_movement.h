#ifndef __SPELL_GROUND_MOVEMENT_H__
#define __SPELL_GROUND_MOVEMENT_H__

#include "../math/vector3.h"
#include "../math/transform_single_axis.h"
#include "../math/vector3.h"
#include <stdbool.h>

struct move_over_ground_def {
    float move_speed;
    float max_upward_movment;
    float height_offset;
    bool can_fly;
};

typedef struct move_over_ground_def move_over_ground_def_t;

bool move_over_ground(struct Vector3* position, struct Vector3* direction, move_over_ground_def_t* definition, float max_downward_movement);

#endif