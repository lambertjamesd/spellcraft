#include "crate.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/box.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../entity/entity_spawner.h"

static struct dynamic_object_type crate_collision_type = {
    .minkowsi_sum = box_minkowski_sum,
    .bounding_box = box_bounding_box,
    .data = {
        .box = {
            .half_size = {0.5f, 0.5f, 0.5f},
        }
    },
    .friction = 0.75f,
    .bounce = 0.0f,
};

void crate_update(struct crate* crate) {
    if (crate->health.current_health <= 0.0f) {
        entity_despawn(crate->health.entity_id);
    }
}

void crate_init(struct crate* crate, struct crate_definition* definition, entity_id id) {
    transformSaInit(&crate->transform, &definition->position, &definition->rotation, definition->scale);

    renderable_single_axis_init(&crate->renderable, &crate->transform, "rom:/meshes/objects/crate.tmesh");
    dynamic_object_init(
        id, 
        &crate->dynamic_object, 
        &crate_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET,
        &crate->transform.position, 
        &crate->transform.rotation
    );
    crate->dynamic_object.scale = definition->scale;
    crate->dynamic_object.weight_class = WEIGHT_CLASS_HEAVY;

    update_add(crate, (update_callback)crate_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable(&crate->renderable, 1.73f);
    collision_scene_add(&crate->dynamic_object);

    health_init(&crate->health, id, 10.0f);
}

void crate_destroy(struct crate* crate) {
    render_scene_remove(&crate->renderable);
    renderable_destroy(&crate->renderable);
    collision_scene_remove(&crate->dynamic_object);
    health_destroy(&crate->health);
    update_remove(crate);
}

void crate_common_init() {

}

void crate_common_destroy() {

}