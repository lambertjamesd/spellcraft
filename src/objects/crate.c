#include "crate.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type create_collision_type = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = dynamic_object_box_bouding_box,
    .data = {
        .box = {
            .half_size = {0.5f, 0.5f, 0.5f},
        }
    }
};

void crate_init(struct crate* crate, struct crate_definition* definition) {
    crate->transform.position = definition->position;
    crate->transform.rotation = definition->rotation;

    renderable_single_axis_init(&crate->renderable, &crate->transform, "rom:/meshes/objects/crate.mesh");
    dynamic_object_init(entity_id_new(), &crate->dynamic_object, &create_collision_type, &crate->transform.position, &crate->transform.rotation);

    crate->render_id = render_scene_add_renderable_single_axis(&r_scene_3d, &crate->renderable, 1.73f);
    // collision_scene_add(&crate->dynamic_object);
}

void crate_destroy(struct crate* crate) {
    render_scene_remove(&r_scene_3d, crate->render_id);
    collision_scene_remove(&crate->dynamic_object);
}