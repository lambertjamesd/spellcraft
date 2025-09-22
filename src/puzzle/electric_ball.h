#ifndef __PUZZLE_ELECTRIC_BALL_H__
#define __PUZZLE_ELECTRIC_BALL_H__

#include <stdbool.h>
#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"

struct electric_ball {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collision;
    health_t health;
    element_attr_t attrs[2];
};

typedef struct electric_ball electric_ball_t;

void electric_ball_init(electric_ball_t* ball, struct electric_ball_definition* definition, entity_id entity_id);
void electric_ball_destroy(electric_ball_t* ball);

void electric_ball_request_ball(struct Vector3* at, entity_id entity_id, bool should_be_lit);
void electric_ball_remove_request(entity_id entity_id);

#endif