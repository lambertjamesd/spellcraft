#include "shield.h"

#include "assets.h"
#include "../time/time.h"
#include "../render/render_scene.h"

#define MAX_SHEILD_LIFETIME 7.0f

void shield_render(void* data, struct render_batch* batch) {
    struct shield* shield = (struct shield*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformToMatrix(&shield->transform, mtx);
    transformApplySceneScale(mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);
    render_batch_add_tmesh(batch, spell_assets_get()->sheild_mesh, mtxfp, 1, NULL, NULL);
}

void shield_update_transform(struct shield* shield) {
    vector3AddScaled(
        &shield->data_source->position, 
        &shield->data_source->direction, 
        shield->hold_radius, 
        &shield->transform.position
    );
    struct Vector3 reverse_dir;
    vector3Negate(&shield->data_source->direction, &reverse_dir);
    quatLook(&reverse_dir, &gUp, &shield->transform.rotation);
}

void shield_init(struct shield* shield, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element) {
    shield->data_source = source;
    spell_data_source_retain(source);
    shield_update_transform(shield);
    shield->transform.scale = gOneVec;
    // TODO get rom target object
    shield->hold_radius = 0.5f;
    shield->lifetime = MAX_SHEILD_LIFETIME;

    shield->start_animation = mesh_animation_new(
        &shield->transform.position, 
        &gRight2, 
        spell_assets_get()->projectile_appear, 
        spell_assets_get()->projectile_appear_clip
    );
}

void shield_destroy(struct shield* shield) {
    if (shield->start_animation) {
        mesh_animation_free(shield->start_animation);
    } else {
        render_scene_remove(shield);
    }
    spell_data_source_release(shield->data_source);
}

bool shield_update(struct shield* shield, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    shield_update_transform(shield);

    if (shield->start_animation) {
        if (mesh_animation_update(shield->start_animation)) {
            shield->start_animation->transform.position = shield->transform.position;
            return true;
        }
        mesh_animation_free(shield->start_animation);
        render_scene_add(&shield->transform.position, 1.0f, shield_render, shield);
        shield->start_animation = NULL;
    }

    shield->lifetime -= fixed_time_step;

    return shield->lifetime >= 0.0f;
}