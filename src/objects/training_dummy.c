#include "training_dummy.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../spell/assets.h"
#include <stddef.h>

#define DUMMY_BURN_TIME 7.0f

static struct dynamic_object_type dummy_collision = {
    .minkowsi_sum = dynamic_object_capsule_minkowski_sum,
    .bounding_box = dynamic_object_capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.35f,
            .inner_half_height = 0.4f,
        }
    }
};

void training_dummy_check_fire(struct training_dummy* dummy) {
    if (health_is_burning(&dummy->health)) {
        if (dummy->burning_effect) {
            burning_effect_refresh(dummy->burning_effect, DUMMY_BURN_TIME);
        } else {
            dummy->burning_effect = burning_effect_new(&dummy->transform.position, 1.5f, DUMMY_BURN_TIME);
            dummy->burning_effect->position.y += 1.0f;
        }
    } else if (dummy->burning_effect) {
        burning_effect_free(dummy->burning_effect);
        dummy->burning_effect = NULL;
    }
}

void training_dummy_update(void* data) {
    struct training_dummy* dummy = (struct training_dummy*)data;

    training_dummy_check_fire(dummy);

    if (health_is_frozen(&dummy->health)) {
        dummy->renderable.force_material = spell_assets_get()->ice_material;
        return;
    } else {
        dummy->renderable.force_material = NULL;
    }

    struct Vector3 dummy_up;
    quatMultVector(&dummy->transform.rotation, &gUp, &dummy_up);

    if (vector3MagSqrd(&dummy->angularVelocity) < 0.0001f && vector3Dot(&dummy_up, &gUp) > 0.9999f) {
        return;
    }

    quatApplyAngularVelocity(&dummy->transform.rotation, &dummy->angularVelocity, fixed_time_step, &dummy->transform.rotation);

    struct Vector3 spring_force;
    vector3Cross(&dummy_up, &gUp, &spring_force);
    vector3AddScaled(&dummy->angularVelocity, &spring_force, 5.0f, &dummy->angularVelocity);
    vector3Scale(&dummy->angularVelocity, &dummy->angularVelocity, 0.7f);
}

void training_dummy_push_damage_hit(struct training_dummy* dummy, entity_id source) {
    struct dynamic_object* object = collision_scene_find_object(source);
    
    if (!object) {
        return;
    }

    struct Vector3 offset;
    vector3Sub(&dummy->transform.position, object->position, &offset);
    offset.y = 0.0f;
    vector3Normalize(&offset, &offset);

    struct Vector3 torque;
    vector3Cross(&offset, &gUp, &torque);
    vector3AddScaled(&dummy->angularVelocity, &torque, -6.0f, &dummy->angularVelocity);
}

void training_dummy_on_hit(void* data, float amount, entity_id source, enum damage_type type) {
    struct training_dummy* dummy = (struct training_dummy*)data;

    if (type & (DAMAGE_TYPE_PROJECTILE | DAMAGE_TYPE_BASH)) {
        training_dummy_push_damage_hit(dummy, source);
    }
}

void training_dummy_init(struct training_dummy* dummy, struct training_dummy_definition* definition) {
    entity_id entity_id = entity_id_new();

    transformInitIdentity(&dummy->transform);
    dummy->transform.position = definition->position;
    quatAxisComplex(&gUp, &definition->rotation, &dummy->transform.rotation);

    renderable_init(&dummy->renderable, &dummy->transform, "rom:/meshes/objects/training_dummy.tmesh");

    render_scene_add_renderable(&dummy->renderable, 2.0f);

    dynamic_object_init(
        entity_id,
        &dummy->collision,
        &dummy_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &dummy->transform.position,
        NULL
    );

    dummy->angularVelocity = gZeroVec;

    dummy->collision.center.y = dummy_collision.data.capsule.inner_half_height + dummy_collision.data.capsule.radius;

    collision_scene_add(&dummy->collision);

    dummy->collision.is_fixed = true;

    health_init(&dummy->health, entity_id, 0.0f);
    health_set_callback(&dummy->health, training_dummy_on_hit, dummy);
    update_add(dummy, training_dummy_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    dummy->burning_effect = NULL;
}

void health_set_callback(struct health* health, health_damage_callback callback, void* data) {
    health->callback = callback;
    health->callback_data = data;
}

void training_dummy_destroy(struct training_dummy* dummy) {
    renderable_destroy(&dummy->renderable);
    collision_scene_remove(&dummy->collision);
    update_remove(dummy);
}