#include "projectile.h"

#include "../render/render_scene.h"
#include "../time/time.h"

#include "assets.h"

void projectile_render(struct projectile* projectile, struct render_batch* batch) {
    mat4x4* mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    render_batch_add_mesh(batch, spell_assets_get()->projectile_mesh, mtx);
}

void projectile_init(struct projectile* projectile, struct spell_data_source* data_source, struct spell_data_source* data_output) {
    projectile->render_id = render_scene_add(&r_scene_3d, &projectile->pos, 0.2f, (render_scene_callback)projectile_render, projectile);

    projectile->data_source = data_source;
    projectile->data_output = data_output;

    projectile->pos = data_source->position;

    data_source->reference_count += 1;
    data_output->reference_count += 1;

    *data_output = *data_source;
}

void projectile_update(struct projectile* projectile) {
    vector3AddScaled(&projectile->pos, &projectile->vel, fixed_time_step, &projectile->pos);

    projectile->data_output->position = projectile->pos;
    projectile->data_output->direction = projectile->data_source->direction;
}

void projectile_destroy(struct projectile* projectile) {
    render_scene_remove(&r_scene_3d, projectile->render_id);
    projectile->render_id = 0;
    projectile->data_source->reference_count -= 1;
    projectile->data_output->reference_count -= 1;
}