#ifndef __ENTITIES_BOSS_DOOR_H__
#define __ENTITIES_BOSS_DOOR_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../objects/door_base.h"

struct boss_door {
    door_base_t door_base;
    boolean_variable is_unlocked;
    scene_entry_point on_interact;
};

typedef struct boss_door boss_door_t;

void boss_door_init(boss_door_t* boss_door, struct boss_door_definition* definition, entity_id entity_id);
void boss_door_destroy(boss_door_t* boss_door, struct boss_door_definition* definition);
void boss_door_common_init();
void boss_door_common_destroy();

#endif