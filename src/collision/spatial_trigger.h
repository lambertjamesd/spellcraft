#ifndef __COLLISION_SPATIAL_TRIGGER_H__
#define __COLLISION_SPATIAL_TRIGGER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/box3d.h"
#include "../entity/entity_id.h"
#include "contact.h"
#include <stdbool.h>

enum SPATIAL_TRIGGER_TYPE {
    SPATIAL_TRIGGER_SPHERE,
    SPATIAL_TRIGGER_CYLINDER,
    SPATIAL_TRIGGER_BOX,
    SPATIAL_TRIGGER_WEDGE,
};

union spatial_trigger_data {
    struct { float radius; } sphere;
    struct { float radius; float half_height; } cylinder;
    struct { struct Vector3 half_size; } box;
    struct { float radius; float half_height; struct Vector2 angle; } wedge;
};

struct spatial_trigger_type {
    enum SPATIAL_TRIGGER_TYPE type;
    union spatial_trigger_data data;
};

struct spatial_trigger {
    struct TransformSingleAxis* transform;
    struct spatial_trigger_type* type;
    struct Box3D bounding_box;
    struct contact* active_contacts;
};

#define SPATIAL_TRIGGER_BOX(x, y, z) .type = SPATIAL_TRIGGER_BOX, .data = {.box = {.half_size = {x,y,z}}}

void spatial_trigger_init(struct spatial_trigger* trigger, struct TransformSingleAxis* transform, struct spatial_trigger_type* type);

void spatial_trigger_recalc_bb(struct spatial_trigger* trigger);
bool spatial_trigger_does_contain_point(struct spatial_trigger* trigger, struct Vector3* point);

#endif