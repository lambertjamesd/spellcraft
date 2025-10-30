#include "ground_torch.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../collision/shapes/cylinder.h"
#include "../render/defs.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include <memory.h>

struct torch_type_def {
    char* mesh_filename;
    char* active_effect;
    enum damage_type start_damage;
    enum damage_type stop_damage;
    struct Vector3 flame_location;
    dynamic_object_type_t collider;
    float collider_offset;
};

static struct torch_type_def torch_defs[] = {
    [GROUND_TORCH_FIRE] = {
        .mesh_filename = "rom:/meshes/objects/torch.tmesh",
        .active_effect = "rom:/meshes/objects/torch_flame.tmesh",
        .start_damage = DAMAGE_TYPE_FIRE,
        .stop_damage = DAMAGE_TYPE_ICE | DAMAGE_TYPE_WATER,
        .flame_location = {0.0f, 0.84124f, 0.0f},
        .collider = {
            CAPSULE_COLLIDER(0.4f, 0.4f),
        },
        .collider_offset = 0.8f,
    },
    [GROUND_TORCH_LIGHTNING] = {
        .mesh_filename = "rom:/meshes/puzzle/electric_torch.tmesh",
        .active_effect = "rom:/meshes/puzzle/torch_lightning.tmesh",
        .start_damage = DAMAGE_TYPE_LIGHTING,
        .stop_damage = DAMAGE_TYPE_WATER,
        .flame_location = {0.0f, 0.84124f, 0.0f},
        .collider = {
            CAPSULE_COLLIDER(0.4f, 0.4f),
        },
        .collider_offset = 0.8f,
    },
    [GROUND_TORCH_KILN] = {
        .mesh_filename = "rom:/meshes/puzzle/kiln.tmesh",
        .active_effect = "rom:/meshes/objects/torch_flame.tmesh",
        .start_damage = DAMAGE_TYPE_FIRE,
        .stop_damage = DAMAGE_TYPE_WATER,
        .flame_location = {0.0f, 0.84124f, 0.0f},
        .collider = {
            CYLINDER_COLLIDER(3.0f, 2.5f),
        },
        .collider_offset = 2.5f,
    },
};

void ground_torch_update(void* data) {
    struct ground_torch* torch = (struct ground_torch*)data;

    struct torch_type_def* def = &torch_defs[torch->torch_type];

    if (health_has_status(&torch->health, def->start_damage)) {
        expression_set_bool(torch->lit_source, true);
        health_clear_status(&torch->health);
    }

    if (health_has_status(&torch->health, def->stop_damage)) {
        expression_set_bool(torch->lit_source, false);
        health_clear_status(&torch->health);
    }
}

void ground_torch_render(void* data, struct render_batch* batch) {
    struct ground_torch* torch = (struct ground_torch*)data;

    T3DMat4FP* mtxfp = render_batch_transformfp_from_sa(batch, &torch->transform);

    if (!mtxfp) {
        return;
    }

    render_batch_add_tmesh(batch, torch->base_mesh, mtxfp, NULL, NULL, NULL);

    if (!expression_get_bool(torch->lit_source)) {
        return;
    }
        
    mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    struct torch_type_def* torch_type = &torch_defs[torch->torch_type];
    
    mat4x4 mtx;

    memcpy(mtx, &batch->camera_matrix, sizeof(mat4x4));
    matrixApplyScaledPos(mtx, &torch->transform.position, WORLD_SCALE);
    matrixApplyScale(mtx, MODEL_WORLD_SCALE);
    mtx[3][1] += torch_type->flame_location.y * WORLD_SCALE;
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, torch->flame_mesh, mtxfp, NULL, NULL, NULL);
}

void ground_torch_init(struct ground_torch* ground_torch, struct ground_torch_definition* definition, entity_id id) {
    transformSaInit(&ground_torch->transform, &definition->position, &definition->rotation, 1.0f);
    ground_torch->torch_type = definition->torch_type;
    
    struct torch_type_def* torch_type = &torch_defs[ground_torch->torch_type];

    dynamic_object_init(
        id, 
        &ground_torch->dynamic_object, 
        &torch_type->collider, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET,
        &ground_torch->transform.position, 
        NULL
    );
    ground_torch->dynamic_object.center.y = torch_type->collider_offset;
    ground_torch->dynamic_object.is_fixed = 1;
    ground_torch->dynamic_object.weight_class = WEIGHT_CLASS_SUPER_HEAVY;


    ground_torch->base_mesh = tmesh_cache_load(torch_defs[definition->torch_type].mesh_filename);
    ground_torch->flame_mesh = tmesh_cache_load(torch_defs[definition->torch_type].active_effect);

    render_scene_add(&ground_torch->transform.position, 1.73f, ground_torch_render, ground_torch);
    collision_scene_add(&ground_torch->dynamic_object);
    health_init(&ground_torch->health, id, 0.0f);
    update_add(ground_torch, ground_torch_update, 1, UPDATE_LAYER_WORLD);

    ground_torch->lit_source = definition->lit_source;
}

void ground_torch_destroy(struct ground_torch* ground_torch) {
    render_scene_remove(ground_torch);
    collision_scene_remove(&ground_torch->dynamic_object);
    update_remove(ground_torch);
    health_destroy(&ground_torch->health);

    tmesh_cache_release(ground_torch->base_mesh);
    tmesh_cache_release(ground_torch->flame_mesh);
}