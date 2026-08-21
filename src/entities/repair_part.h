#ifndef __ENTITIES_REPAIR_PART_H__
#define __ENTITIES_REPAIR_PART_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "entity_deps.h"

struct repair_part {
    transform_t transform;
    renderable_t renderable;
    entity_id entity_id;
};

typedef struct repair_part repair_part_t;

void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id);
void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition);
void repair_part_common_init();
void repair_part_common_destroy();

extern void* repair_part_raycast_position;

#endif