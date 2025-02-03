#include "living_sprite.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../render/defs.h"
#include "../collision/shapes/capsule.h"
#include "assets.h"

static struct dynamic_object_type living_sprite_collision = {
    .minkowsi_sum = capsule_minkowski_sum,
    .bounding_box = capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.25f,
            .inner_half_height = 0.1f,
        }
    },
    .friction = 0.5f,
    .bounce = 0.5f,
};

void living_sprite_render(void* data, struct render_batch* batch) {
    struct living_sprite* living_sprite = (struct living_sprite*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(&living_sprite->transform, mtx, 1.0f);
    mtx[3][0] *= SCENE_SCALE;
    mtx[3][1] *= SCENE_SCALE;
    mtx[3][2] *= SCENE_SCALE;
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->fire_sprite, mtxfp, 1, NULL, NULL);
}

void living_sprite_init(struct living_sprite* living_sprite, struct spell_data_source* source, struct spell_event_options event_options) {
    entity_id entity_id = entity_id_new();

    vector3AddScaled(&source->position, &source->direction, 0.5f, &living_sprite->transform.position);
    living_sprite->transform.rotation = gRight2;

    render_scene_add(&living_sprite->transform.position, 0.5f, living_sprite_render, living_sprite);

    dynamic_object_init(
        entity_id,
        &living_sprite->collider,
        &living_sprite_collision,
        COLLISION_LAYER_TANGIBLE,
        &living_sprite->transform.position,
        &living_sprite->transform.rotation
    );

    living_sprite->collider.center.y = living_sprite_collision.data.capsule.radius + living_sprite_collision.data.capsule.inner_half_height;

    collision_scene_add(&living_sprite->collider);
}

bool living_sprite_update(struct living_sprite* living_sprite, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    return true;
}

void living_sprite_destroy(struct living_sprite* living_sprite) {
    render_scene_remove(living_sprite);
}