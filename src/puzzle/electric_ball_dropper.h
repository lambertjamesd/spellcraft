#ifndef __PUZZLE_ELECTRIC_BALL_DROPPER_H__
#define __PUZZLE_ELECTRIC_BALL_DROPPER_H__

#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"
#include "../render/renderable.h"

struct electric_ball_dropper {
    transform_sa_t transform;
    renderable_t renderable;
    entity_id current_ball;
    boolean_variable is_active;
};

typedef struct electric_ball_dropper electric_ball_dropper_t;

void electric_ball_dropper_init(electric_ball_dropper_t* dropper, struct electric_ball_dropper_definition* definition, entity_id entity_id);
void electric_ball_dropper_destroy(electric_ball_dropper_t* dropper);

#endif