#include "biter.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../resource/animation_cache.h"

#define VISION_DISTANCE 4.0f

static struct Vector2 biter_max_rotation;

static struct dynamic_object_type biter_collision_type = {
    .minkowsi_sum = dynamic_object_sphere_minkowski_sum,
    .bounding_box = dynamic_object_sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.4f,
        },
    },
};

static struct dynamic_object_type biter_vision_collision_type = {
    .minkowsi_sum = dynamic_object_cone_minkowski_sum,
    .bounding_box = dynamic_object_cone_bounding_box,
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

void biter_update(struct biter* biter) {
    animator_update(&biter->animator, biter->renderable.armature.pose, fixed_time_step);

    struct contact* nearest_target = dynamic_object_nearest_contact(&biter->vision);

    if (nearest_target) {
        struct Vector3 target;
        target = nearest_target->point;

        struct Vector2 direction;
        direction.y = biter->transform.position.x - target.x;
        direction.x = target.z - biter->transform.position.z;

        vector2Normalize(&direction, &direction);
        vector2RotateTowards(&biter->transform.rotation, &direction, &biter_max_rotation, &biter->transform.rotation);
    }


    if (biter->health.current_health <= 0.0f) {
        biter_destroy(biter);
    }
}

void biter_init(struct biter* biter, struct biter_definition* definition) {
    biter->transform.position = definition->position;
    biter->transform.rotation = definition->rotation;

    entity_id id = entity_id_new();

    renderable_single_axis_init(&biter->renderable, &biter->transform, "rom:/meshes/enemies/enemy1.mesh");
    dynamic_object_init(
        id, 
        &biter->dynamic_object, 
        &biter_collision_type, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    dynamic_object_init(
        id, 
        &biter->vision, 
        &biter_vision_collision_type, 
        COLLISION_LAYER_DAMAGE_PLAYER,
        &biter->transform.position, 
        &biter->transform.rotation
    );

    biter->dynamic_object.center.y = biter_collision_type.data.sphere.radius * 0.5f;
    biter->vision.center.y = biter->dynamic_object.center.y;
    biter->vision.is_trigger = 1;

    collision_scene_add(&biter->dynamic_object);
    collision_scene_add(&biter->vision);

    update_add(biter, (update_callback)biter_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    render_scene_add_renderable_single_axis(&r_scene_3d, &biter->renderable, 1.73f);

    health_init(&biter->health, id, 10.0f);

    biter->animation_set = animation_cache_load("rom:/meshes/enemies/enemy1.anim");
    biter->animations.attack = animation_set_find_clip(biter->animation_set, "enemy1_attack1");
    biter->animations.idle = animation_set_find_clip(biter->animation_set, "enemy1_idle");
    biter->animations.run = animation_set_find_clip(biter->animation_set, "enemy1_walk");

    animator_init(&biter->animator, biter->renderable.armature.bone_count);

    animator_run_clip(&biter->animator, biter->animations.idle, 0.0f, true);

    vector2ComplexFromAngle(fixed_time_step * 3.0f, &biter_max_rotation);
}

void biter_destroy(struct biter* biter) {
    render_scene_remove(&r_scene_3d, &biter->renderable);
    collision_scene_remove(&biter->dynamic_object);
    collision_scene_remove(&biter->vision);
    health_destroy(&biter->health);
    update_remove(biter);
}