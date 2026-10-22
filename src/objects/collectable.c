#include "collectable.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/expression_evaluate.h"
#include "../cutscene/show_item.h"
#include "../menu/dialog_box.h"
#include "../player/inventory.h"
#include "../render/render_scene.h"
#include "../resource/material_cache.h"
#include "../spell/spell.h"
#include "../time/time.h"
#include "../util/hash_map.h"
#include "../entity/entity_spawner.h"
#include "../player/player.h"

#define COLLECTABLE_RADIUS  0.75f

static struct dynamic_object_type collectable_collision = {
    .minkowsi_sum = sphere_minkowski_sum,
    .bounding_box = sphere_bounding_box,
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
    char* message;
};

static struct collectable_information collectable_information[] = {
    [COLLECTABLE_TYPE_HEALTH] = {
        .mesh_filename = "rom:/meshes/objects/pickups/heart.tmesh",
        .message = "You found a health upgrade.\n\nYour max health is permanantly increased."
    },
    [COLLECTABLE_TYPE_MANA_UPGRADE] = {
        .mesh_filename = "rom:/meshes/objects/pickups/manabottle.tmesh",
        .message = "You found a mana upgrade.\n\nYour max mana is permanantly increased."
    },
    [COLLECTABLE_TYPE_SPELL_SCROLL] = {
        .mesh_filename = "rom:/meshes/objects/pickups/scroll.tmesh",
        .message = "You found a spell scroll.\n\nUse them to cast full spells with a single C button press while holding R."
    },
};

void collectable_assets_load() {
    hash_map_init(&collectable_hash_map, 8);
}

void collectable_init(struct collectable* collectable, struct collectable_definition* definition, entity_id id) {
    collectable->collectable_type = definition->collectable_type;
    collectable->has_item = definition->has_item;
    transformSaInit(&collectable->transform, &definition->position, &definition->rotation, 1.0f);
    
    dynamic_object_init(
        id, 
        &collectable->dynamic_object, 
        &collectable_collision, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &collectable->transform.position, 
        0
    );

    struct collectable_information* type = &collectable_information[definition->collectable_type];

    collision_scene_add(&collectable->dynamic_object);
    renderable_single_axis_init(&collectable->renderable, &collectable->transform, type->mesh_filename);
    render_scene_add_renderable(&collectable->renderable, 0.2f);
    
    hash_map_set(&collectable_hash_map, collectable->dynamic_object.entity_id, collectable);
}

static vector3_t relative_player_pos = {
    0.393962f,
    1.16192f,
    0.481766f,
};

void collectable_float_above_hand(void* data) {
    collectable_t* collectable = (collectable_t*)data;

    collectable->renderable.hide = false;

    transformSaTransformPoint(&player_get()->cutscene_actor.transform, &relative_player_pos, &collectable->transform.position);
    collectable->transform.scale = 0.5f;
    health_t* health = &player_get()->health;
    health->max_health += 5.0f;
    health->current_health = health->max_health;
}

void collectable_collected(struct collectable* collectable) {
    if (collectable->has_item != VARIABLE_DISCONNECTED) {
        expression_set_bool(collectable->has_item, true);
    }

    struct cutscene_builder builder;

    collectable->dynamic_object.is_fixed = true;
    collectable->dynamic_object.weight_class = WEIGHT_CLASS_GHOST;
    collectable->dynamic_object.collision_layers = 0;
    collectable->dynamic_object.has_gravity = false;
    collectable->dynamic_object.velocity = gZeroVec;
    collectable->renderable.hide = true;
    cutscene_builder_init(&builder);
    cutscene_builder_pause(&builder, true, false);
    cutscene_builder_npc_animate(&builder, ENTITY_ID_PLAYER, "item_get", false);
    cutscene_builder_camera_animate(&builder, "item_get", ENTITY_ID_PLAYER, 0);

    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_callback(&builder, collectable_float_above_hand, collectable);

    cutscene_builder_camera_wait(&builder);
    cutscene_builder_npc_animate(&builder, ENTITY_ID_PLAYER, "item_get_loop", true);
    cutscene_builder_dialog(
        &builder,
        collectable_information[collectable->collectable_type].message
    );
    cutscene_builder_npc_animate(&builder, ENTITY_ID_PLAYER, "item_get_store", false);
    cutscene_builder_callback(&builder, (cutscene_init_callback)entity_despawn, (void*)(int)collectable->dynamic_object.entity_id);
    cutscene_builder_camera_return(&builder);
    cutscene_builder_pause(&builder, false, false);
    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        0,
        cutscene_runner_free_on_finish(),
        NULL,
        collectable->dynamic_object.entity_id
    );
}

void collectable_destroy(struct collectable* collectable) {
    collision_scene_remove(&collectable->dynamic_object);
    render_scene_remove(&collectable->renderable);
    renderable_destroy(&collectable->renderable);
    hash_map_delete(&collectable_hash_map, collectable->dynamic_object.entity_id);
}

void collectable_common_init() {
    hash_map_init(&collectable_hash_map, 8);
}

void collectable_common_destroy() {
    hash_map_destroy(&collectable_hash_map);
}

struct collectable* collectable_get(entity_id id) {
    return hash_map_get(&collectable_hash_map, id);
}