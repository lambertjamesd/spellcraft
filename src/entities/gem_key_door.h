#ifndef __ENTITIES_GEM_KEY_DOOR_H__
#define __ENTITIES_GEM_KEY_DOOR_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

#include "../objects/door_base.h"

struct gem_key_door {
    door_base_t door_base;
    boolean_variable unlocked;
    integer_variable key_gems;
    uint8_t gem_count;
};

typedef struct gem_key_door gem_key_door_t;

void gem_key_door_init(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition, entity_id entity_id);
void gem_key_door_destroy(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition);
void gem_key_door_common_init();
void gem_key_door_common_destroy();

#endif