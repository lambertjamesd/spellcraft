#include "repair_part.h"    

#include "../screen_raycast/screen_raycast.h"
#include "../resource/tmesh_cache.h"

void* repair_part_raycast_position;

void repair_part_do_render(void* data, struct render_batch* batch) {
    repair_part_t* repair_part = (repair_part_t*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformToWorldMatrix(&repair_part->transform, mtx);

    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    if (repair_part_raycast_position) {
        screen_raycast_check_pixel(repair_part_raycast_position);
    }

    t3d_matrix_push(mtxfp);

    rspq_block_run(repair_part->mesh->block);      

    t3d_matrix_pop(1);


    if (repair_part_raycast_position) {
        screen_raycast_check_changed(repair_part_raycast_position, repair_part->entity_id);
    }
}

void repair_part_render(void* data, struct render_batch* batch) {
    repair_part_t* repair_part = (repair_part_t*)data;

    render_batch_add_callback(batch, repair_part->mesh->material, repair_part_do_render, repair_part);
}
    
void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id) {
    transformInit(&repair_part->transform, &definition->start_position, &definition->start_rotation, &gOneVec);

    repair_part->mesh = tmesh_cache_load(definition->mesh);
    render_scene_add(&repair_part->transform.position, 4.0f, repair_part_render, repair_part);

    repair_part->entity_id = entity_id;
}

void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition) {
    render_scene_remove(repair_part);
    tmesh_cache_release(repair_part->mesh);
}

void repair_part_common_init() {

}

void repair_part_common_destroy() {

}
