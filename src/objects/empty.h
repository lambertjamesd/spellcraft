#ifndef __OBJECTS_EMPTY_H__
#define __OBJECTS_EMPTY_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"

struct empty_definition {
    struct Vector3 position;
};

struct empty {
    struct Vector3 position;
};

void empty_init(struct empty* empty, struct empty_definition* definition, entity_id entity_id);
void empty_destroy(struct empty* empty, struct empty_definition* definition);

#endif