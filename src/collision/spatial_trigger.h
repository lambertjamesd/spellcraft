#ifndef __COLLISION_SPATIAL_TRIGGER_H__
#define __COLLISION_SPATIAL_TRIGGER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector3.h"
#include "../math/box3d.h"
#include "contact.h"
#include <stdbool.h>

enum SPATIAL_TRIGGER_TYPE {
    SPATIAL_TRIGGER_SPHERE,
    SPATIAL_TRIGGER_CYLINDER,
    SPATIAL_TRIGGER_BOX,
};

union spatial_trigger_data
{
    struct { float radius; } sphere;
    struct { float radius; float half_height; } cylinder;
    struct { struct Vector3 half_size; } box;
};

struct spatial_trigger {
    struct TransformSingleAxis* transform;
    enum SPATIAL_TRIGGER_TYPE type;
    union spatial_trigger_data data;
    struct Box3D bounding_box;
    struct contact* active_contacts;
};

void spatial_trigger_recalc_bb(struct spatial_trigger* trigger);
bool spatial_trigger_does_contain_point(struct spatial_trigger* trigger, struct Vector3* point);

#endif