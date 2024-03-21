#include "crate.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type crate_collision_type = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = dynamic_object_box_bouding_box,
    .data = {
        .box = {
            .half_size = {0.5f, 0.5f, 0.5f},
        }
    }
};

void crate_update(struct crate* crate) {
    if (crate->health.current_health <= 0.0f) {
        crate_destroy(crate);
    }
}

void crate_init(struct crate* crate, struct crate_definition* definition) {
    crate->transform.position = definition->position;
    crate->transform.rotation = definition->rotation;

    entity_id id = entity_id_new();

    renderable_single_axis_init(&crate->renderable, &crate->transform, "rom:/meshes/objects/crate.mesh");
    dynamic_object_init(
        id, 
        &crate->dynamic_object, 
        &crate_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &crate->transform.position, 
        &crate->transform.rotation
    );

    update_add(crate, (update_callback)crate_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable_single_axis(&r_scene_3d, &crate->renderable, 1.73f);
    collision_scene_add(&crate->dynamic_object);

    health_init(&crate->health, id, 10.0f);
}

void crate_destroy(struct crate* crate) {
    render_scene_remove(&r_scene_3d, &crate->renderable);
    collision_scene_remove(&crate->dynamic_object);
    health_destroy(&crate->health);
    update_remove(crate);
}