#include "water_waves.h"    

#include "../menu/menu_rendering.h"

static tmesh_t* mesh_test;

void water_waves_render(void* data, render_batch_t* render_batch) {
    water_waves_t* water_waves = (water_waves_t*)data;
    water_simulation_apply(water_waves->mesh, &water_waves->position);

    T3DMat4FP* mtx = render_batch_transformfp_from_point(render_batch, &water_waves->position);

    if (!mtx) {
        return;
    }

    render_batch_add_tmesh(render_batch, water_waves->mesh, mtx, NULL, NULL, NULL);
}

    
void water_waves_init(water_waves_t* water_waves, struct water_waves_definition* definition, entity_id entity_id) {
    water_waves->position = definition->position;
    render_scene_add(&water_waves->position, mesh_test->radius, &water_waves_render, water_waves);
    water_waves->mesh = mesh_test;
}

void water_waves_destroy(water_waves_t* water_waves, struct water_waves_definition* definition) {
    render_scene_remove(water_waves);
}

void water_waves_common_init() {
    water_simulation_retain();
    mesh_test = tmesh_cache_load("rom:/meshes/test/water_sim.tmesh");

    water_simulation_enable_debug_render();
}

void water_waves_common_destroy() {
    water_simulation_release();

    tmesh_cache_release(mesh_test);
    
    water_simulation_disable_debug_render();
}
