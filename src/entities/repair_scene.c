#include "repair_scene.h"    
    
void repair_scene_init(repair_scene_t* repair_scene, struct repair_scene_definition* definition, entity_id entity_id) {
    repair_scene->position = definition->position;
}

void repair_scene_destroy(repair_scene_t* repair_scene, struct repair_scene_definition* definition) {
    
}

void repair_scene_common_init() {

}

void repair_scene_common_destroy() {

}
