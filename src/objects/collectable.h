#ifndef __OBJECTS_COLLECTABLE_H__
#define __OBJECTS_COLLECTABLE_H__

#include "../math/vector3.h"
#include "../collision/dynamic_object.h"
#include "../scene/world_definition.h"

struct collectable {
    struct Vector3 position;
    struct dynamic_object dynamic_object;
};

void collectable_init(struct collectable* collectable, struct collectable_definition* definition);
void collectable_destroy(struct collectable* collectable);

#endif