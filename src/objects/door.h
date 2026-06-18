#ifndef __OBJECTS_DOOR_H__
#define __OBJECTS_DOOR_H__

#include "door_base.h"

struct door {
    door_base_t door_base;
    boolean_variable unlocked;
};

void door_init(struct door* door, struct door_definition* definition, entity_id id);
void door_destroy(struct door* door);
void door_common_init();
void door_common_destroy();

#endif