#include "biter.h"

#include "../collision/collision_scene.h"
#include "../collision/shapes/cone.h"
#include "../collision/shapes/sphere.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

#define VISION_DISTANCE     4.0f
#define ATTACK_RANGE        1.0f
#define MOVE_SPEED          6.5f
#define MOVE_ACCELERATION   8.0f

static struct Vector2 biter_max_rotation;

static struct dynamic_object_type biter_collision_type = {
    .minkowsi_sum = sphere_minkowski_sum,
    .bounding_box = sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.4f,
        },
    },
    .friction = 0.1f,
    // about a 40 degree slope
    .max_stable_slope = 0.219131191f,
};

static struct dynamic_object_type biter_vision_collision_type = {
    .minkowsi_sum = cone_minkowski_sum,
    .bounding_box = cone_bounding_box,
    .data = {
        .sphere = {
            .radius = VISION_DISTANCE,
        },
    },
    .data = {
        .cone = {
            .size = {VISION_DISTANCE, VISION_DISTANCE, VISION_DISTANCE},
        },
    },
};

void biter_update_target(struct biter* biter) {
    if (biter->animator.current_clip == biter->animations.attack) {
        if (!biter->current_target || !biter->animator.events) {
            return;
        }

        struct dynamic_object* target_object = collision_scene_find_object(biter->current_target);

        if (!target_object) {
            return;
        }
        
        if (vector3DistSqrd(target_object->position, &biter->transform.position) > ATTACK_RANGE * ATTACK_RANGE) {
            return;
        }

        struct damage_info damage = {
            .amount = 1.0f,
            .type = DAMAGE_TYPE_BASH,
            .source = biter->dynamic_object.entity_id,
        };
        vector2ToLookDir(&biter->transform.rotation, &damage.direction);
        health_damage_id(biter->current_target, &damage);
        biter->current_target = 0;
        return;
    }

    if (!biter->current_target) {
        struct contact* nearest_target = dynamic_object_nearest_contact(&biter->vision);

        if (nearest_target) {
            biter->current_target = nearest_target->other_object;
        }
    }

    if (!biter->current_target) {
        if (biter->animator.current_clip != biter->animations.idle) {
            animator_run_clip(&biter->animator, biter->animations.idle, 0.0f, true);
        }

        return;
    }

    struct dynamic_object* target_object = collision_scene_find_object(biter->current_target);

    struct Vector3 direction;
    struct Vector2 rotation;

    vector3Sub(target_object->position, &biter->transform.position, &direction);
    
    float distance_sqrd = vector3MagSqrd(&direction);

    if (distance_sqrd > VISION_DISTANCE * VISION_DISTANCE) {
        biter->current_target = 0;
        return;
    }

    bool should_attack = distance_sqrd < ATTACK_RANGE * ATTACK_RANGE || dynamic_object_find_contact(&biter->dynamic_object, biter->current_target);

    vector2LookDir(&rotation, &direction);

    float angle_dot = vector2Dot(&rotation, &biter->transform.rotation);

    if (should_attack && angle_dot > 0.9) {
        animator_run_clip(&biter->animator, biter->animations.attack, 0.0f, false);
        return;
    }

    vector2RotateTowards(&biter->transform.rotation, &rotation, &biter_max_rotation, &biter->transform.rotation);

    if (should_attack) {
        // stop moving to attack
        return;
    }

    if (angle_dot > 0.0f && !biter->dynamic_object.is_pushed) {
        struct Vector3 targetVelocity;
        vector3RotatedSpeed(&biter->transform.rotation, &targetVelocity, MOVE_SPEED);
        vector3MoveTowards(&biter->dynamic_object.velocity, &targetVelocity, MOVE_ACCELERATION * fixed_time_step, &biter->dynamic_object.velocity);

        if (biter->animator.current_clip != biter->animations.run) {
            animator_run_clip(&biter->animator, biter->animations.run, 0.0f, true);
        }
    }
}

void biter_update(struct biter* biter) {
    animator_update(&biter->animator, biter->renderable.armature.pose, fixed_time_step);

    biter_update_target(biter);

    if (biter->health.current_health <= 0.0f) {
        biter_destroy(biter);
    }
}

void biter_init(struct biter* biter, struct biter_definition* definition) {
    biter->transform.position = definition->position;
    biter->transform.rotation = definition->rotation;

    entity_id id = entity_id_new();
    entity_id vision_id = entity_id_new();

    renderable_single_axis_init(&biter->renderable, &biter->transform, "rom:/meshes/enemies/enemy1.tmesh");
    dynamic_object_init(
        id, 
        &biter->dynamic_object, 
        &biter_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    dynamic_object_init(
        vision_id, 
        &biter->vision, 
        &biter_vision_collision_type, 
        COLLISION_LAYER_DAMAGE_PLAYER,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    biter->dynamic_object.center.y = biter_collision_type.data.sphere.radius * 0.5f;
    biter->dynamic_object.density_class = DYNAMIC_DENSITY_MEDIUM;
    biter->vision.center.y = biter->dynamic_object.center.y;
    biter->vision.is_trigger = 1;

    collision_scene_add(&biter->dynamic_object);
    collision_scene_add(&biter->vision);

    update_add(biter, (update_callback)biter_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable(&biter->renderable, 1.73f);

    health_init(&biter->health, id, 10.0f);

    biter->animation_set = animation_cache_load("rom:/meshes/enemies/enemy1.anim");
    biter->animations.attack = animation_set_find_clip(biter->animation_set, "enemy1_attack");
    biter->animations.idle = animation_set_find_clip(biter->animation_set, "enemy1_idle");
    biter->animations.run = animation_set_find_clip(biter->animation_set, "enemy1_walk");
    biter->current_target = 0;

    animator_init(&biter->animator, biter->renderable.armature.bone_count);

    animator_run_clip(&biter->animator, biter->animations.idle, 0.0f, true);

    vector2ComplexFromAngle(fixed_time_step * 3.0f, &biter_max_rotation);
}

void biter_destroy(struct biter* biter) {
    render_scene_remove(&biter->renderable);
    renderable_destroy(&biter->renderable);
    collision_scene_remove(&biter->dynamic_object);
    collision_scene_remove(&biter->vision);
    health_destroy(&biter->health);
    update_remove(biter);
    animator_destroy(&biter->animator);
    animation_cache_release(biter->animation_set);
}