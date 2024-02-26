#include "projectile.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

#include "assets.h"

static struct dynamic_object_type player_collision = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = 0,
    .data = {
        .box = {
            .half_size = {0.25f, 0.25f, 0.25f},
        }
    }
};

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

    dynamic_object_init(&projectile->dynamic_object, &player_collision, &projectile->pos, NULL);
    collision_scene_add(&projectile->dynamic_object);

    projectile->dynamic_object.velocity = data_source->direction;
}

void projectile_update(struct projectile* projectile) {
    projectile->data_output->position = projectile->pos;
    projectile->data_output->direction = projectile->data_source->direction;

    if (projectile->dynamic_object.active_contacts) {
        // TODO trigger collision
    }
}

void projectile_destroy(struct projectile* projectile) {
    render_scene_remove(&r_scene_3d, projectile->render_id);
    projectile->render_id = 0;
    projectile->data_source->reference_count -= 1;
    projectile->data_output->reference_count -= 1;
    collision_scene_remove(&projectile->dynamic_object);
}