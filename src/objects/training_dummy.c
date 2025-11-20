#include "training_dummy.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../render/render_scene.h"
#include "../spell/assets.h"
#include "../time/time.h"
#include "../math/mathf.h"
#include <stddef.h>

#define DUMMY_BURN_TIME 7.0f

static struct dynamic_object_type dummy_collision = {
    .minkowsi_sum = capsule_minkowski_sum,
    .bounding_box = capsule_bounding_box,
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

void training_dummy_push(struct training_dummy* dummy, struct Vector3* direction, float strength) {
    struct Vector3 torque;
    vector3Cross(direction, &gUp, &torque);
    vector3AddScaled(&dummy->angularVelocity, &torque, -6.0f, &dummy->angularVelocity);
}

void training_dummy_update(void* data) {
    struct training_dummy* dummy = (struct training_dummy*)data;

    training_dummy_check_fire(dummy);

    if (health_is_shocked(&dummy->health)) {
        if (dummy->shock_timer == 0) {
            struct Vector3 prevForward;
            quatMultVector(&dummy->transform.rotation, &gForward, &prevForward);
            vector3Negate(&prevForward, &prevForward);

            prevForward.y = randomInRangef(-0.25f, 0.25f);
            
            struct Vector3 randomUp = (struct Vector3){randomInRangef(-0.25f, 0.25f), 1.0f, randomInRangef(-0.25f, 0.25f)};
            quatLook(&prevForward, &randomUp, &dummy->transform.rotation);

            dummy->angularVelocity = gZeroVec;
            dummy->shock_timer = 2;
        } else {
            --dummy->shock_timer;
            dummy->renderable.force_material = (dummy->shock_timer & 1) ? spell_assets_get()->shock_material : NULL;
        }
        return;
    } else if (health_is_frozen(&dummy->health)) {
        dummy->renderable.force_material = spell_assets_get()->ice_material;
        return;
    } else {
        dummy->renderable.force_material = NULL;
        dummy->shock_timer = 0;

        if (vector3Dot(&dummy->collision.velocity, &dummy->collision.velocity) > 0.0f) {
            training_dummy_push(dummy, &dummy->collision.velocity, 20.0f);
            dummy->collision.velocity = gZeroVec;
        }
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

void training_dummy_push_damage_hit(struct training_dummy* dummy, struct damage_info* damage) {
    struct Vector3 offset = damage->direction;
    offset.y = 0.0f;
    vector3Normalize(&offset, &offset);
    training_dummy_push(dummy, &offset, -6.0f);
}

float training_dummy_on_hit(void* data, struct damage_info* damage) {
    struct training_dummy* dummy = (struct training_dummy*)data;

    if (damage->type & (DAMAGE_TYPE_PROJECTILE | DAMAGE_TYPE_BASH)) {
        training_dummy_push_damage_hit(dummy, damage);
    }

    return damage->amount;
}

void training_dummy_init(struct training_dummy* dummy, struct training_dummy_definition* definition, entity_id id) {
    transformInitIdentity(&dummy->transform);
    dummy->transform.position = definition->position;
    quatAxisComplex(&gUp, &definition->rotation, &dummy->transform.rotation);

    renderable_init(&dummy->renderable, &dummy->transform, "rom:/meshes/objects/training_dummy.tmesh");

    render_scene_add_renderable(&dummy->renderable, 2.0f);

    dynamic_object_init(
        id,
        &dummy->collision,
        &dummy_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET,
        &dummy->transform.position,
        NULL
    );

    dummy->angularVelocity = gZeroVec;
    dummy->shock_timer = 0;

    dummy->collision.center.y = dummy_collision.data.capsule.inner_half_height + dummy_collision.data.capsule.radius;

    collision_scene_add(&dummy->collision);

    dummy->collision.is_fixed = true;

    health_init(&dummy->health, id, 0.0f);
    health_set_callback(&dummy->health, training_dummy_on_hit, dummy);
    update_add(dummy, training_dummy_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    dummy->burning_effect = NULL;
}

void training_dummy_destroy(struct training_dummy* dummy) {
    render_scene_remove(&dummy->renderable);
    renderable_destroy(&dummy->renderable);
    collision_scene_remove(&dummy->collision);
    update_remove(dummy);
    health_destroy(&dummy->health);
}

void training_dummy_common_init() {

}

void training_dummy_common_destroy() {

}