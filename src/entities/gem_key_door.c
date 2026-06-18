#include "gem_key_door.h"    
    
void gem_key_door_init(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition, entity_id entity_id) {
    gem_key_door->position = definition->position;
}

void gem_key_door_destroy(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition) {
    
}

void gem_key_door_common_init() {

}

void gem_key_door_common_destroy() {

}
