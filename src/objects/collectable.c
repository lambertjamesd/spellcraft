#include "collectable.h"

#include "../util/hash_map.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../resource/material_cache.h"

#include "../menu/dialog_box.h"

#define COLLECTABLE_RADIUS  0.5f

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

void collectable_assets_load() {
    collectable_assets.collectable_test = material_cache_load("rom:/materials/objects/collectable.mat");

    hash_map_init(&collectable_hash_map, 8);
}

void collectable_init(struct collectable* collectable, struct collectable_definition* definition) {
    collectable->type = COLLECTABLE_TYPE_HEALTH;
    collectable->transform.position = definition->position;
    collectable->transform.rotation = gRight2;
    
    dynamic_object_init(
        entity_id_new(), 
        &collectable->dynamic_object, 
        &collectable_collision, 
        COLLISION_LAYER_TANGIBLE,
        &collectable->transform.position, 
        0
    );

    collectable->dynamic_object.center.y = COLLECTABLE_RADIUS;

    collision_scene_add(&collectable->dynamic_object);
    renderable_single_axis_init(&collectable->renderable, &collectable->transform, "rom:/meshes/objects/pickups/heart.mesh");
    render_scene_add_renderable_single_axis(&r_scene_3d, &collectable->renderable, 0.2f);
    
    hash_map_set(&collectable_hash_map, collectable->dynamic_object.entity_id, collectable);
}

void collectable_collected(struct collectable* collectable) {
    collectable_destroy(collectable);

    dialog_box_show(&g_dialog_box, 
        "You got a heart\n\n"
        "Now if I only had a brain",
        NULL, NULL
    );
}

void collectable_destroy(struct collectable* collectable) {
    collision_scene_remove(&collectable->dynamic_object);
    render_scene_remove(&r_scene_3d, &collectable->renderable);
    renderable_single_axis_destroy(&collectable->renderable);
    hash_map_delete(&collectable_hash_map, collectable->dynamic_object.entity_id);
}

struct collectable* collectable_get(entity_id id) {
    return hash_map_get(&collectable_hash_map, id);
}