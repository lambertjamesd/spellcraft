#include "camera_wall_checker.h"

#include <stddef.h>
#include <math.h>
#include "camera_controller.h"
#include "../collision/shapes/sphere.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"

#define MAX_CAST_DISTANCE   6.0f

static struct dynamic_object_type camera_wall_checker_type = {
    SPHERE_COLLIDER(0.25f),
};

void camera_wall_checker_init(camera_wall_checker_t* checker) {
    dynamic_object_init(
        entity_id_new(),
        &checker->collider,
        &camera_wall_checker_type,
        COLLISION_LAYER_TANGIBLE,
        &checker->position,
        NULL
    );
    checker->collider.collision_group = COLLISION_GROUP_PLAYER;
    checker->collider.weight_class = WEIGHT_CLASS_GHOST;
    checker->collider.has_gravity = false;
    checker->position = gZeroVec;
    checker->cast_from = gZeroVec;
    collision_scene_add(&checker->collider);
}

void camera_wall_checker_update(camera_wall_checker_t* checker, vector3_t* look_target, vector3_t* position) {
    checker->actual_distance = sqrtf(vector3DistSqrd(&checker->position, &checker->cast_from));

    checker->position = *look_target;
    checker->cast_from = *look_target;
    vector3Sub(position, look_target, &checker->collider.velocity);

    float target_distnace = vector3MagSqrd(&checker->collider.velocity);

    if (target_distnace > MAX_CAST_DISTANCE * MAX_CAST_DISTANCE) {
        vector3Scale(&checker->collider.velocity, &checker->collider.velocity, MAX_CAST_DISTANCE / sqrt(target_distnace));
    } else if (target_distnace < CAMERA_FOLLOW_DISTANCE * CAMERA_FOLLOW_DISTANCE) {
        vector3Scale(&checker->collider.velocity, &checker->collider.velocity, CAMERA_FOLLOW_DISTANCE / sqrt(target_distnace));\
    }

    vector3Scale(&checker->collider.velocity, &checker->collider.velocity, 1.0f / fixed_time_step);
}

void camera_wall_checker_destroy(camera_wall_checker_t* checker) {
    collision_scene_remove(&checker->collider);
}
