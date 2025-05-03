#ifndef __ENTITY_SHIELD_H__
#define __ENTITY_SHIELD_H__

#include <stdbool.h>
#include "../math/vector3.h"
#include "damage.h"

struct health_shield {
    struct Vector3 direction;
};

void health_shield_init(struct health_shield* health_shield, struct Vector3* direction);
bool health_shield_does_block(struct health_shield* health_shield, struct damage_info* damage);

#endif 