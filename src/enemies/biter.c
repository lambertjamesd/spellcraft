#include "biter.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type biter_collision_type = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = dynamic_object_box_bouding_box,
    .data = {
        .box = {
            .half_size = {0.5f, 0.5f, 0.5f},
        }
    }
};

void biter_update(struct biter* biter) {
    if (biter->health.current_health <= 0.0f) {
        biter_destroy(biter);
    }
}

void biter_init(struct biter* biter, struct biter_definition* definition) {
    biter->transform.position = definition->position;
    biter->transform.rotation = definition->rotation;

    entity_id id = entity_id_new();

    renderable_single_axis_init(&biter->renderable, &biter->transform, "rom:/meshes/enemies/enemy1.mesh");
    dynamic_object_init(
        id, 
        &biter->dynamic_object, 
        &biter_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISOIN_LAYER_DAMAGE_ENEMY,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    update_add(biter, (update_callback)biter_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable_single_axis(&r_scene_3d, &biter->renderable, 1.73f);
    // collision_scene_add(&biter->dynamic_object);

    health_init(&biter->health, id, 10.0f);
}

void biter_destroy(struct biter* biter) {
    render_scene_remove(&r_scene_3d, &biter->renderable);
    // collision_scene_remove(&biter->dynamic_object);
    health_destroy(&biter->health);
    update_remove(biter);
}