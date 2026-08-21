#include "repair_scene.h"    

#include "../scene/scene.h"
#include "../render/defs.h"
    
void repair_scene_init(repair_scene_t* repair_scene, struct repair_scene_definition* definition, entity_id entity_id) {
    camera_init(&repair_scene->camera, definition->fov, WORLD_NEAR_PLANE, WORLD_FAR_PLANE);
    transformInit(&repair_scene->camera.transform, &definition->position, &definition->rotation, &gOneVec);
    player_disable(&current_scene->player);
    render_scene_use_camera(&repair_scene->camera);
}

void repair_scene_destroy(repair_scene_t* repair_scene, struct repair_scene_definition* definition) {
    render_scene_remove_camera(&repair_scene->camera);
}

void repair_scene_common_init() {

}

void repair_scene_common_destroy() {

}
