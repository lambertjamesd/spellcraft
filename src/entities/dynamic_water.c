#include "dynamic_water.h"

#include "../objects/water_cube.h"
#include "../water/water.h"

#define CHANGE_RATE     1.0f

void dynamic_water_render(void* data, render_batch_t* batch) {
    dynamic_water_t* water = (dynamic_water_t*)data;
    water_simulation_apply(water->mesh, &water->min);

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &water->transform);

    if (!mtx) {
        return;
    }

    render_batch_add_tmesh(batch, water->mesh, mtx, NULL, NULL, NULL);
}

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

    dynamic_water->mesh = tmesh_cache_load(definition->mesh);
    render_scene_add(&dynamic_water->transform.position, dynamic_water->mesh->radius, dynamic_water_render, dynamic_water);

    spatial_trigger_type_from_shape(&dynamic_water->trigger_type, &definition->collider);
    spatial_trigger_init(&dynamic_water->trigger, &dynamic_water->transform, &dynamic_water->trigger_type, COLLISION_LAYER_TANGIBLE, entity_id);
    collision_scene_add_trigger(&dynamic_water->trigger);

    vector3s16_t min;
    vector3s16_t max;
    tmesh_compute_bounding_box(dynamic_water->mesh, &min, &max);
    dynamic_water->min.x = dynamic_water->transform.position.x + min.x * (1.0f / MODEL_SCALE);
    dynamic_water->min.y = dynamic_water->transform.position.y + min.y * (1.0f / MODEL_SCALE);
    dynamic_water->min.z = dynamic_water->transform.position.z + min.z * (1.0f / MODEL_SCALE);

    update_add(dynamic_water, dynamic_water_update, UPDATE_PRIORITY_PHYICS | UPDATE_LAYER_CUTSCENE, UPDATE_LAYER_WORLD);
}

void dynamic_water_destroy(dynamic_water_t* dynamic_water, struct dynamic_water_definition* definition) {
    render_scene_remove(dynamic_water);
    tmesh_cache_release(dynamic_water->mesh);
    collision_scene_remove_trigger(&dynamic_water->trigger);
    update_remove(dynamic_water);
}

void dynamic_water_common_init() {
    water_simulation_retain();

    water_simulation_enable_debug_render();
}

void dynamic_water_common_destroy() {
    water_simulation_release();

    water_simulation_disable_debug_render();
}
