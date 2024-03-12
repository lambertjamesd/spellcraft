#ifndef __RENDER_ARMATURE_H__
#define __RENDER_ARMATURE_H__

#include <stdint.h>
#include "../math/vector3.h"
#include "../math/transform.h"

struct armature_packed_transform {
    int16_t x, y, z;
    int16_t rx, ry, rz; // w is derived from the other values
    int16_t sx, sy, sz;
};

#define NO_BONE_PARENT  0xFF

struct armature_definition {
    uint16_t bone_count;
    uint8_t* parent_linkage;
    struct armature_packed_transform* default_pose;
};

struct armature {
    uint16_t bone_count;
    uint8_t* parent_linkage;
    struct Transform* pose;
};

void armature_definition_init(struct armature_definition* definition, int boune_count);
void armature_definition_destroy(struct armature_definition* definition);

void armature_init(struct armature* armature, struct armature_definition* definition);
void armature_destroy(struct armature* armature);

#endif