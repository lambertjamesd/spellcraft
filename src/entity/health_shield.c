#include "health_shield.h"

void health_shield_init(struct health_shield* health_shield, struct Vector3* direction) {
    health_shield->direction = *direction;
}

bool health_shield_does_block(struct health_shield* health_shield, struct damage_info* damage) {
    return vector3Dot(&health_shield->direction, &damage->direction) < 0.0f;
}