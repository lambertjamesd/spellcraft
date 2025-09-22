#ifndef __PUZZLE_ELECTRIC_BALL_GRABBER_H__
#define __PUZZLE_ELECTRIC_BALL_GRABBER_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../entity/entity_id.h"
#include "../render/renderable.h"
#include "../collision/spatial_trigger.h"

struct electric_ball_grabber {
    transform_sa_t transform;
    renderable_t renderable;
    spatial_trigger_t trigger;
    float has_ball_time;
    boolean_variable output;
    entity_id entity_id;
    element_attr_t attrs[2];
};

typedef struct electric_ball_grabber electric_ball_grabber_t;

void electric_ball_grabber_init(electric_ball_grabber_t* grabber, struct electric_ball_grabber_definition* definition, entity_id entity_id);
void electric_ball_grabber_destroy(electric_ball_grabber_t* grabber);

#endif