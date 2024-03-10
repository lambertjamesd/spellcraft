#include "collectable.h"

#include "../util/hash_map.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../resource/material_cache.h"

#define COLLECTABLE_RADIUS  0.25f

static struct dynamic_object_type collectable_collision = {
    .minkowsi_sum = dynamic_object_sphere_minkowski_sum,
    .bounding_box = dynamic_object_sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = COLLECTABLE_RADIUS,
        }
    },
    .bounce = 0.4f,
    .friction = 0.25f,
};

struct collectable_assets {
    struct material* collectable_test;
};

static struct hash_map collectable_hash_map;

struct collectable_assets collectable_assets;

void collectable_render(struct collectable* collectable, struct render_batch* batch) {
    struct render_batch_billboard_element* element = render_batch_add_particles(batch, collectable_assets.collectable_test, 1);

    element->sprites->position = collectable->position;
    element->sprites->radius = COLLECTABLE_RADIUS;
    element->sprites->color.r = 255;
    element->sprites->color.g = 255;
    element->sprites->color.b = 255;
    element->sprites->color.a = 255;
}

void collectable_assets_load() {
    collectable_assets.collectable_test = material_cache_load("rom:/materials/objects/collectable.mat");

    hash_map_init(&collectable_hash_map, 8);
}

void collectable_init(struct collectable* collectable, struct collectable_definition* definition) {
    collectable->type = COLLECTABLE_TYPE_HEALTH;
    collectable->position = definition->position;
    
    dynamic_object_init(
        entity_id_new(), 
        &collectable->dynamic_object, 
        &collectable_collision, 
        COLLISION_LAYER_TANGIBLE,
        &collectable->position, 
        0
    );
    collision_scene_add(&collectable->dynamic_object);
    render_scene_add(&r_scene_3d, &collectable->position, 0.2f, (render_scene_callback)collectable_render, collectable);
    
    hash_map_set(&collectable_hash_map, collectable->dynamic_object.entity_id, collectable);
}

void collectable_collected(struct collectable* collectable) {
    collectable_destroy(collectable);
}

void collectable_destroy(struct collectable* collectable) {
    collision_scene_remove(&collectable->dynamic_object);
    render_scene_remove(&r_scene_3d, collectable);
    hash_map_delete(&collectable_hash_map, collectable->dynamic_object.entity_id);
}

struct collectable* collectable_get(entity_id id) {
    return hash_map_get(&collectable_hash_map, id);
}