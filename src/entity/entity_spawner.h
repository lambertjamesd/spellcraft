#ifndef __ENTITY_ENTITY_SPAWNER_H__
#define __ENTITY_ENTITY_SPAWNER_H__

#include "../scene/scene_definition.h"
#include "entity_id.h"

entity_id entity_spawn(enum entity_type_id type, void* definition);
void entity_despawn(entity_id entity_id);

#endif