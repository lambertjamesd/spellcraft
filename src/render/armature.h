#ifndef __RENDER_ARMATURE_H__
#define __RENDER_ARMATURE_H__

#include <stdint.h>
#include "../math/vector3.h"
#include "../math/transform.h"
#include "armature_definition.h"

#define NO_BONE_PARENT  0xFF

struct armature {
    uint8_t* parent_linkage;
    struct Transform* pose;
    uint16_t bone_count;
    uint8_t image_frame_0;
    uint8_t image_frame_1;
};

void armature_definition_init(struct armature_definition* definition, int boune_count);
void armature_definition_destroy(struct armature_definition* definition);

void armature_init(struct armature* armature, struct armature_definition* definition);
void armature_destroy(struct armature* armature);

#endif