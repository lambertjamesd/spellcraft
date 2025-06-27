#include "jelly.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"
#include "../collision/shapes/box.h"
#include "../entity/entity_id.h"
#include "../math/minmax.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"
#include "../render/defs.h"

#define MAX_HEALTH      400.0f
#define STARTING_HEALTH 20.0f
#define HEAL_RATE       30.0f

#define STARTING_RADIUS 0.4f
#define MIN_RADIUS      0.2f

#define FREEZE_TIME     1.0f

#define JUMP_INTERVAL   4.0f
#define JUMP_IMPULSE    4.0f
#define JUMP_WINDUP     0.9f
#define JUMP_TIME       0.2f

static struct dynamic_object_type jelly_collider = {
    SPHERE_COLLIDER(1.0f),
    .friction = 0.1f,
    .bounce = 0.7f,
    // about a 40 degree slope
    .max_stable_slope = 0.219131191f,
};

static struct dynamic_object_type jelly_ice_collider = {
    BOX_COLLIDER(1.0f, 1.0f, 1.0f),
    .friction = 0.025f,
    .bounce = 0.7f,
    // about a 40 degree slope
    .max_stable_slope = 0.0f,
};

void jelly_freeze(struct jelly* jelly) {
    jelly->is_frozen = 1;
    jelly->freeze_timer = 0.0f;
    jelly->collider.type = &jelly_ice_collider;
    jelly->collider.density_class = DYNAMIC_DENSITY_MEDIUM;
}

void jelly_thaw(struct jelly* jelly) {
    jelly->is_frozen = 0;
    jelly->freeze_timer = 0.0f;
    jelly->collider.type = &jelly_collider;
    jelly->collider.density_class = DYNAMIC_DENSITY_NEUTRAL;
}

float jelly_recalc_radius(struct jelly* jelly) {
    float health_ratio = sqrtf(MAX(jelly->health.current_health, 0.0f) * (1.0f / STARTING_HEALTH));
    return (health_ratio * (STARTING_RADIUS - MIN_RADIUS)) + MIN_RADIUS;
}

float jelly_on_hit(void* data, struct damage_info* damage) {
    struct jelly* jelly = (struct jelly*)data;

    if (jelly->is_frozen) {
        if (damage->type & (DAMAGE_TYPE_BASH | DAMAGE_TYPE_PROJECTILE)) {
            return damage->amount;
        }

        if (damage->type & DAMAGE_TYPE_FIRE) {
            jelly->freeze_timer += fixed_time_step;

            if (jelly->freeze_timer > FREEZE_TIME) {
                jelly_thaw(jelly);
            }
            return 0.0f;
        }

        return 0.0f;
    }
    
    jelly->needs_new_radius = 1;

    if (damage->type & DAMAGE_TYPE_WATER) {
        health_heal(&jelly->health, fixed_time_step * HEAL_RATE);
        return 0.0f;
    }

    if (damage->type & DAMAGE_TYPE_ICE) {
        jelly->freeze_timer += fixed_time_step;

        if (jelly->freeze_timer > FREEZE_TIME) {
            jelly_freeze(jelly);
        }

        return 0.0f;
    }

    if (damage->type & DAMAGE_TYPE_STEAL) {
        return jelly->health.current_health;
    }

    if (damage->type & (DAMAGE_TYPE_FIRE | DAMAGE_TYPE_LIGHTING)) {
        return damage->amount;
    }

    return 0.0f;
}

void jelly_render(void* data, struct render_batch* batch) {
    struct jelly* jelly = (struct jelly*)data;
    
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(&jelly->transform, mtx, jelly->collider.scale);

    struct Vector3 shear_direction;
    vector3Sub(&jelly->shear_spring, &jelly->transform.position, &shear_direction);
    if (shear_direction.y < 0.1f) {
        shear_direction.y = 0.1f;
    }
    float side_scale = 1.0f / sqrtf(shear_direction.y);
    vector3Scale(&shear_direction, &shear_direction, jelly->collider.scale * MODEL_WORLD_SCALE);
    mtx[0][0] *= side_scale;
    mtx[0][2] *= side_scale;
    mtx[1][0] = shear_direction.x;
    mtx[1][1] = shear_direction.y;
    mtx[1][2] = shear_direction.z;
    mtx[2][0] *= side_scale;
    mtx[2][2] *= side_scale;

    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(
        batch, 
        jelly->is_frozen ? jelly->ice_mesh : jelly->mesh, 
        mtxfp, 
        1, 
        NULL, 
        NULL
    );
}

void jelly_update_spring(struct jelly* jelly) {
    struct Vector3 target_spring;
    vector3Add(&jelly->transform.position, &gUp, &target_spring);

    float time_left = JUMP_INTERVAL - jelly->jump_timer;

    if (time_left < JUMP_WINDUP + JUMP_TIME) {
        if (time_left < JUMP_TIME) {
            target_spring.y += 0.1f;
        } else {
            target_spring.y -= 0.5f - (time_left - JUMP_TIME) * (0.5f / JUMP_WINDUP);
        }
    }

    struct Vector3 offset;
    vector3Sub(&target_spring, &jelly->shear_spring, &offset);

    vector3AddScaled(&jelly->shear_velocity, &offset, 2.5f, &jelly->shear_velocity);
    vector3Scale(&jelly->shear_velocity, &jelly->shear_velocity, 0.91f);
    vector3AddScaled(&jelly->shear_spring, &jelly->shear_velocity, fixed_time_step, &jelly->shear_spring);
}

void jelly_update(void* data) {
    struct jelly* jelly = (struct jelly*)data;

    jelly_update_spring(jelly);

    if (jelly->needs_new_radius) {
        dynamic_object_set_scale(&jelly->collider, jelly_recalc_radius(jelly));
        jelly->collider.center.y = jelly->collider.scale;
        jelly->needs_new_radius = 0;
    }

    if (dynamic_object_is_grounded(&jelly->collider)) {
        if (jelly->jump_timer > JUMP_INTERVAL) {
            jelly->collider.velocity.y += JUMP_IMPULSE;
            jelly->jump_timer = 0.0f;
        }
    
        jelly->jump_timer += fixed_time_step;
    }

    if (jelly->health.current_health <= 0.0f || (!jelly->is_frozen && jelly->collider.under_water)) {
        jelly_destroy(jelly);
    }
}

void jelly_init(struct jelly* jelly, struct jelly_definition* definition) {
    entity_id entity_id = entity_id_new();

    jelly->transform.position = definition->position;
    jelly->transform.rotation = definition->rotation;

    jelly->mesh = tmesh_cache_load("rom:/meshes/enemies/water_jelly.tmesh");
    jelly->ice_mesh = tmesh_cache_load("rom:/meshes/enemies/ice_jelly.tmesh");

    render_scene_add(&jelly->transform.position, 1.0f, jelly_render, jelly);

    health_init(&jelly->health, entity_id, MAX_HEALTH);
    health_set_callback(&jelly->health, jelly_on_hit, jelly);
    jelly->health.current_health = STARTING_HEALTH;
    jelly->needs_new_radius = 0;
    jelly->is_frozen = 0;

    jelly->freeze_timer = 0.0f;
    vector3Add(&definition->position, &gUp, &jelly->shear_spring);
    jelly->shear_velocity = gZeroVec;

    dynamic_object_init(
        entity_id, 
        &jelly->collider, 
        &jelly_collider, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &jelly->transform.position,
        &jelly->transform.rotation
    );

    jelly->collider.density_class = DYNAMIC_DENSITY_NEUTRAL;
    jelly->collider.scale = jelly_recalc_radius(jelly);
    jelly->collider.center.y = jelly->collider.scale;
    jelly->jump_timer = 0.0f;

    collision_scene_add(&jelly->collider);

    update_add(jelly, jelly_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);
}

void jelly_destroy(struct jelly* jelly) {
    render_scene_remove(jelly);
    health_destroy(&jelly->health);
    collision_scene_remove(&jelly->collider);
    update_remove(jelly);
    tmesh_cache_release(jelly->mesh);
    tmesh_cache_release(jelly->ice_mesh);
}