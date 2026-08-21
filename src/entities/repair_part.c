#include "repair_part.h"    
    
void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id) {
    transformInit(&repair_part->transform, &definition->start_position, &definition->start_rotation, &gOneVec);

    renderable_init(&repair_part->renderable, &repair_part->transform, definition->mesh);
    render_scene_add_renderable(&repair_part->renderable, 4.0f);
}

void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition) {
    render_scene_remove(&repair_part->renderable);
    renderable_destroy(&repair_part->renderable);
}

void repair_part_common_init() {

}

void repair_part_common_destroy() {

}
