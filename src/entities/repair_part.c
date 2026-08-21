#include "repair_part.h"    

#include "../screen_raycast/screen_raycast.h"

void* repair_part_raycast_position;

void repair_part_render(void* data, struct render_batch* batch) {
    repair_part_t* repair_part = (repair_part_t*)data;
    
    if (repair_part_raycast_position) {
        screen_raycast_check_pixel(repair_part_raycast_position);
    }
    render_scene_render_renderable(&repair_part->renderable, batch);
    if (repair_part_raycast_position) {
        screen_raycast_check_changed(repair_part_raycast_position, repair_part->entity_id);
    }
}
    
void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id) {
    transformInit(&repair_part->transform, &definition->start_position, &definition->start_rotation, &gOneVec);

    renderable_init(&repair_part->renderable, &repair_part->transform, definition->mesh);
    render_scene_add_renderable(&repair_part->renderable, 4.0f);
    render_scene_add(&repair_part->transform.position, 4.0f, repair_part_render, repair_part);
}

void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition) {
    render_scene_remove(&repair_part->renderable);
    renderable_destroy(&repair_part->renderable);
}

void repair_part_common_init() {

}

void repair_part_common_destroy() {

}
