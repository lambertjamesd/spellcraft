#ifndef __OBJECTS_WATER_H__
#define __OBJECTS_WATER_H__

#include "../math/transform_single_axis.h"
#include "../collision/spatial_trigger.h"
#include "../scene/scene_definition.h"

struct water_cube {
    struct TransformSingleAxis transform;
    struct spatial_trigger trigger;
    struct spatial_trigger_type trigger_type;
};

void water_cube_init(struct water_cube* cube, struct water_cube_definition* definition);
void water_cube_destroy(struct water_cube* cube);

#endif