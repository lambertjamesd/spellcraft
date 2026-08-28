#include "dynamic_water.h"

#include "../objects/water_cube.h"

#define CHANGE_RATE     1.0f

void dynamic_water_update(void* data) {
    dynamic_water_t* dynamic_water = (dynamic_water_t*)data;
    water_cube_apply_water(&dynamic_water->trigger);

    float target = expression_get_bool(dynamic_water->is_other_level) ? dynamic_water->other_level : dynamic_water->start_level;

    dynamic_water->transform.position.y = mathfMoveTowards(
        dynamic_water->transform.position.y,
        target,
        CHANGE_RATE * fixed_time_step
    );
}
    
void dynamic_water_init(dynamic_water_t* dynamic_water, struct dynamic_water_definition* definition, entity_id entity_id) {
    transformSaInit(&dynamic_water->transform, &definition->position, &gRight2, 1.0f);

    dynamic_water->start_level = definition->position.y;
    dynamic_water->other_level = definition->other_level.y;
    dynamic_water->is_other_level = definition->is_other_level;

    dynamic_water->transform.position.y = expression_get_bool(dynamic_water->is_other_level) ? dynamic_water->other_level : dynamic_water->start_level;

    renderable_single_axis_init(&dynamic_water->renderable, &dynamic_water->transform, definition->mesh);
    render_scene_add_renderable(&dynamic_water->renderable, 0.0f);

    spatial_trigger_type_from_shape(&dynamic_water->trigger_type, &definition->collider);
    spatial_trigger_init(&dynamic_water->trigger, &dynamic_water->transform, &dynamic_water->trigger_type, COLLISION_LAYER_TANGIBLE, entity_id);
    collision_scene_add_trigger(&dynamic_water->trigger);

    update_add(dynamic_water, dynamic_water_update, UPDATE_PRIORITY_PHYICS | UPDATE_LAYER_CUTSCENE, UPDATE_LAYER_WORLD);
}

void dynamic_water_destroy(dynamic_water_t* dynamic_water, struct dynamic_water_definition* definition) {
    render_scene_remove(&dynamic_water->renderable);
    renderable_destroy(&dynamic_water->renderable);
    collision_scene_remove_trigger(&dynamic_water->trigger);
    update_remove(dynamic_water);
}

void dynamic_water_common_init() {

}

void dynamic_water_common_destroy() {

}
