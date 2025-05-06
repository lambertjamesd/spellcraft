#ifndef __ENTITY_DAMAGE_H__
#define __ENTITY_DAMAGE_H__

#include "../math/vector3.h"
#include "./entity_id.h"

enum damage_type {
    DAMAGE_TYPE_PROJECTILE = (1 << 0),
    DAMAGE_TYPE_FIRE = (1 << 1),
    DAMAGE_TYPE_ICE = (1 << 2),
    DAMAGE_TYPE_LIGHTING = (1 << 3),
    DAMAGE_TYPE_WATER = (1 << 4),
    DAMAGE_TYPE_BASH = (1 << 5),
    DAMAGE_TYPE_STEAL = (1 << 6),
};

struct damage_info {
    float amount;
    enum damage_type type;
    entity_id source;
    // may not be normalized
    struct Vector3 direction;
};

#endif