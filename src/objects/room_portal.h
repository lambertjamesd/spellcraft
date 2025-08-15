#ifndef __OBJECTS_ROOM_PORTAL_H__
#define __OBJECTS_ROOM_PORTAL_H__

#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../render/renderable.h"

struct room_portal {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    room_id room_a;
    room_id room_b;
    float last_player_distance;

    room_id current_room;

    struct element_attr attrs[2];
};

void room_portal_init(struct room_portal* portal, struct room_portal_definition* definition);
void room_portal_destroy(struct room_portal* portal);

#endif