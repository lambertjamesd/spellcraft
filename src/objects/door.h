#ifndef __OBJECTS_DOOR_H__
#define __OBJECTS_DOOR_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"

struct door {
    struct TransformSingleAxis transform;
};

void door_init(struct door* door, struct door_definition* definition);
void door_destroy(struct door* door);

#endif