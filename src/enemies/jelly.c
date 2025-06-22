#include "jelly.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"
#include "../collision/shapes/box.h"
#include "../entity/entity_id.h"
#include "../math/minmax.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"

#define MAX_HEALTH      400.0f
#define STARTING_HEALTH 20.0f
#define HEAL_RATE       30.0f

#define STARTING_RADIUS 0.4f
#define MIN_RADIUS      0.2f

#define FREEZE_TIME     1.0f

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

    T3DMat4FP* mtxfp = render_batch_transformfp_from_sa(batch, &jelly->transform, 32.0f);

    if (!mtxfp) {
        return;
    }

    render_batch_add_tmesh(
        batch, 
        jelly->is_frozen ? jelly->ice_mesh : jelly->mesh, 
        mtxfp, 
        1, 
        NULL, 
        NULL
    );
}

void jelly_update(void* data) {
    struct jelly* jelly = (struct jelly*)data;

    if (jelly->is_frozen) {

    }

    if (jelly->needs_new_radius) {
        dynamic_object_set_scale(&jelly->collider, jelly_recalc_radius(jelly));
        jelly->collider.center.y = jelly->collider.scale;
        jelly->needs_new_radius = 0;
    }

    if (jelly->health.current_health <= 0.0f) {
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