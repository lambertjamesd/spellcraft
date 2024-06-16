#ifndef __RENDER_ARMATURE_H__
#define __RENDER_ARMATURE_H__

#include <stdint.h>
#include <t3d/t3d.h>
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
    // frames can trigger events
    uint16_t active_events;
};

void armature_definition_init(struct armature_definition* definition, int boune_count);
void armature_definition_destroy(struct armature_definition* definition);

void armature_init(struct armature* armature, struct armature_definition* definition);
void armature_destroy(struct armature* armature);

// this wont be needed once instancing is working
void armature_def_apply(struct armature_definition* definition, T3DMat4FP* pose);

#endif