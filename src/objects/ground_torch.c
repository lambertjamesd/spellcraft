#include "ground_torch.h"


#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../resource/mesh_cache.h"

#define TORCH_HEIGHT    0.84124f

static struct dynamic_object_type ground_torch_collision_type = {
    .minkowsi_sum = dynamic_object_capsule_minkowski_sum,
    .bounding_box = dynamic_object_capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.4f,
            .inner_half_height = 0.1f,
        },
    }
};

void ground_torch_render(void* data, struct render_batch* batch) {
    struct ground_torch* torch = (struct ground_torch*)data;

    mat4x4* mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    matrixFromPosition(*mtx, &torch->position);

    render_batch_add_mesh(batch, torch->base_mesh, mtx, NULL);

    struct Vector3 flame_position = torch->position;
    flame_position.y += TORCH_HEIGHT;
        
    mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    matrixFromPosition(*mtx, &flame_position);

    render_batch_add_mesh(batch, torch->flame_mesh, mtx, NULL);
}

void ground_torch_init(struct ground_torch* ground_torch, struct ground_torch_definition* definition) {
    ground_torch->position = definition->position;

    entity_id id = entity_id_new();

    dynamic_object_init(
        id, 
        &ground_torch->dynamic_object, 
        &ground_torch_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &ground_torch->position, 
        NULL
    );
    ground_torch->dynamic_object.center.y = 0.5f;
    ground_torch->dynamic_object.is_fixed = 1;

    ground_torch->base_mesh = mesh_cache_load("rom:/meshes/objects/torch.mesh");
    ground_torch->flame_mesh = mesh_cache_load("rom:/meshes/objects/torch_flame.mesh");

    render_scene_add(&r_scene_3d, &ground_torch->position, 1.73f, ground_torch_render, ground_torch);
    collision_scene_add(&ground_torch->dynamic_object);
}

void ground_torch_destroy(struct ground_torch* ground_torch) {
    render_scene_remove(&r_scene_3d, ground_torch);
    collision_scene_remove(&ground_torch->dynamic_object);
}