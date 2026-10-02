#include "camera_wall_checker.h"

#include <stddef.h>
#include <math.h>
#include "camera_controller.h"
#include "../collision/shapes/sphere.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../scene/scene_definition.h"

#define MIN_CAST_DISTANCE   0.25f
#define MAX_CAST_DISTANCE   6.0f

static struct dynamic_object_type camera_wall_checker_type = {
    SPHERE_COLLIDER(0.25f),
};

void camera_wall_checker_init(camera_wall_checker_t* checker) {
    checker->collider.collision_group = COLLISION_GROUP_PLAYER;
    checker->collider.weight_class = WEIGHT_CLASS_GHOST;
    checker->collider.has_gravity = false;
    checker->position = gZeroVec;
    checker->cast_from = gZeroVec;
    checker->actual_distance = MAX_CAST_DISTANCE;
    dynamic_object_init(
        entity_id_new(),
        &checker->collider,
        &camera_wall_checker_type,
        COLLISION_LAYER_BLOCK_CAMERA,
        &checker->position,
        NULL
    );
    checker->collider.collision_group = ENTITY_ID_PLAYER;
    collision_scene_add(&checker->collider);
}

contact_t* camera_wall_checker_get_floor_contact(camera_wall_checker_t* checker) {
    if (checker->collider.shadow_contact) {
        return checker->collider.shadow_contact;
    }
    
    return dynamic_object_get_ground(&checker->collider);
}

void camera_wall_checker_update(camera_wall_checker_t* checker, vector3_t* look_target, vector3_t* position, float follow_distance) {
    float start_y = checker->position.y;
    checker->actual_distance = sqrtf(vector3DistSqrd(&checker->position, &checker->cast_from));

    checker->position = *look_target;
    checker->cast_from = *look_target;
    struct Vector3 direction;
    vector3Sub(position, look_target, &direction);
    vector3Normalize(&direction, &direction);

    vector3AddScaled(look_target, &direction, MIN_CAST_DISTANCE, &checker->position);
    checker->cast_from = checker->position;
    
    vector3Scale(&direction, &checker->collider.velocity, follow_distance / fixed_time_step);
    dynamic_object_wake(&checker->collider);
    // stop it from sticking to the floor
    checker->collider.is_jumping = true;

    contact_t* floor = camera_wall_checker_get_floor_contact(checker);

    if (floor) {
        checker->floor_height = start_y - floor->point.y;
    } else {
        checker->floor_height = 100.0f;
    }
}

void camera_wall_checker_destroy(camera_wall_checker_t* checker) {
    collision_scene_remove(&checker->collider);
}