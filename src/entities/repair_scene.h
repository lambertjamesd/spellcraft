#ifndef __ENTITIES_REPAIR_SCENE_H__
#define __ENTITIES_REPAIR_SCENE_H__

#include "entity_deps.h"
#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../render/camera.h"

struct repair_scene {
    camera_t camera;
};

typedef struct repair_scene repair_scene_t;

void repair_scene_init(repair_scene_t* repair_scene, struct repair_scene_definition* definition, entity_id entity_id);
void repair_scene_destroy(repair_scene_t* repair_scene, struct repair_scene_definition* definition);
void repair_scene_common_init();
void repair_scene_common_destroy();

#endif