#include "collectable.h"

#include "../util/hash_map.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../resource/material_cache.h"

#include "../menu/dialog_box.h"

#define COLLECTABLE_RADIUS  0.75f

static struct dynamic_object_type collectable_collision = {
    .minkowsi_sum = dynamic_object_sphere_minkowski_sum,
    .bounding_box = dynamic_object_sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = COLLECTABLE_RADIUS,
        }
    },
    .bounce = 0.2f,
    .friction = 0.25f,
};

static struct hash_map collectable_hash_map;

struct collectable_information {
    char* mesh_filename;
};

static struct collectable_information collectable_information[] = {
    [COLLECTABLE_TYPE_HEALTH] = {
        .mesh_filename = "rom:/meshes/objects/pickups/heart.mesh",
    },
    [COLLECTABLE_TYPE_SPELL_RUNE] = {
        .mesh_filename = "rom:/meshes/objects/pickups/scroll.mesh",
    },
};

void collectable_assets_load() {
    hash_map_init(&collectable_hash_map, 8);
}

void collectable_init(struct collectable* collectable, struct collectable_definition* definition) {
    collectable->type = definition->collectable_type;
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

    struct collectable_information* type = &collectable_information[definition->collectable_type];

    // collectable->dynamic_object.center.y = COLLECTABLE_RADIUS;

    collision_scene_add(&collectable->dynamic_object);
    renderable_single_axis_init(&collectable->renderable, &collectable->transform, type->mesh_filename);
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