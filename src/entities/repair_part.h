#ifndef __ENTITIES_REPAIR_PART_H__
#define __ENTITIES_REPAIR_PART_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"
#include "../render/tmesh.h"
#include "../math/ray.h"

enum repair_part_status {
    REPAIR_MISSING_PARTS,
    REPAIR_INCOMPLETE,
    REPAIR_COMPLETE,
};

typedef enum repair_part_status repair_part_status_t;

struct repair_part {
    transform_t transform;
    quaternion_t target_rotation;

    transform_t end_transform;

    tmesh_t* mesh;
    entity_id entity_id;

    bool is_connected;
    bool is_present;
    bool prevent_rotation;

    entity_spawner depends_on;
};

typedef struct repair_part repair_part_t;

void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id);
void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition);
void repair_part_common_init();
void repair_part_common_destroy();

repair_part_t* repair_part_get(entity_id id);

enum repair_part_status repair_part_status();

void repair_part_connect(repair_part_t* repair_part);
bool repair_part_can_connect(repair_part_t* repair_part);
void repair_part_render_drop_location(repair_part_t* part, struct render_batch* batch);

bool repair_part_is_in_right_spot(repair_part_t* repair_part, ray_t* ray_check);

#endif