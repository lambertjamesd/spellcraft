#ifndef __RENDER_ARMATURE_H__
#define __RENDER_ARMATURE_H__

#include <stdint.h>
#include "../math/vector3.h"
#include "../math/transform.h"

struct armature_packed_transform {
    int16_t x, y, z;
    int16_t rx, ry, rz; // w is derived from these
    int16_t sx, sy, sz;
};

struct armature {
    uint16_t bone_count;
    uint8_t* parent_linkage;
    struct Transform* pose;
};

#endif