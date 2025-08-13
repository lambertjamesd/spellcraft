#include "shield.h"

#include "assets.h"
#include "../time/time.h"
#include "../render/render_scene.h"

#define MAX_SHEILD_LIFETIME 7.0f
#define PARRY_WINDOW        0.5f

void shield_render(void* data, struct render_batch* batch) {
    struct shield* shield = (struct shield*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformToWorldMatrix(&shield->transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);
    render_batch_add_tmesh(batch, spell_assets_get()->sheild_mesh, mtxfp, NULL, NULL, NULL);
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

bool shield_should_block(void *data, struct damage_info* damage) {
    struct shield* shield = (struct shield*)data;

    if (shield->parry_timer > 0.0f) {
        shield->flags.did_parry = true;
        shield->parry_timer = 0.0f;
    }

    if (shield->element == ELEMENT_TYPE_ICE) {
        shield->lifetime = 0.0f;
    }

    return true;
}

void shield_init(struct shield* shield, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element) {
    shield->data_source = source;
    spell_data_source_retain(source);
    shield_update_transform(shield);
    shield->transform.scale = gOneVec;
    // TODO get rom target object
    shield->hold_radius = 0.5f;
    shield->lifetime = element == ELEMENT_TYPE_FIRE ? PARRY_WINDOW : MAX_SHEILD_LIFETIME;
    shield->element = element;
    shield->parry_timer = element == ELEMENT_TYPE_ICE ? MAX_SHEILD_LIFETIME : PARRY_WINDOW;
    shield->flags.did_parry = false;

    shield->start_animation = mesh_animation_new(
        &shield->transform.position, 
        &gRight2, 
        spell_assets_get()->projectile_appear, 
        spell_assets_get()->projectile_appear_clip
    );

    struct health* shielding_health = health_get(source->target);
    health_shield_init(&shield->shield, &source->direction, shield_should_block, shield);

    if (shielding_health) {
        health_add_shield(shielding_health, &shield->shield);
    }
}

void shield_destroy(struct shield* shield) {
    if (shield->start_animation) {
        mesh_animation_free(shield->start_animation);
    } else {
        render_scene_remove(shield);
    }
    spell_data_source_release(shield->data_source);

    struct health* shielding_health = health_get(shield->data_source->target);

    if (shielding_health) {
        health_remove_shield(shielding_health, &shield->shield);
    }
}

bool shield_update(struct shield* shield, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    shield_update_transform(shield);
    shield->shield.direction = shield->data_source->direction;

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
    shield->parry_timer -= fixed_time_step;

    if (shield->flags.did_parry) {
        spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, shield->data_source, 0.0f);
        shield->flags.did_parry = false;
    }

    return shield->lifetime >= 0.0f;
}