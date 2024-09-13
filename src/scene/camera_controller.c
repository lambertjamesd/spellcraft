#include "camera_controller.h"

#include "../time/time.h"
#include "../math/transform_single_axis.h"
#include "../physics/move_towards.h"
#include <math.h>

#define CAMERA_FOLLOW_DISTANCE  3.4f
#define CAMERA_FOLLOW_HEIGHT    1.6f

static struct move_towards_parameters camera_move_parameters = {
    .max_speed = 100.0f,
    .max_accel = 20.0f,
};

#define ASPECT_RATIO    (4.0f/3.0f)
#define OVER_SHOULDER_DISTANCE  1.1f

#define MIN_TWO_TARGET_DISTANCE 

void camera_cached_calcuations_check(struct camera_cached_calcuations* cache, struct Camera* camera) {
    if (camera->fov == cache->fov) {
        return;
    }

    float height = tanf(camera->fov * 0.5f);
    float width = ASPECT_RATIO * height;

    cache->fov_horz = atanf(width) * 2.0f;
    cache->cos_1_3_fov_horz = cosf(cache->fov_horz * 0.33333f);
    cache->sin_1_3_fov_horz = sinf(cache->fov_horz * 0.33333f);
}

void camera_controller_determine_two_target_position(struct camera_controller* controller, struct Vector3* result) {
    camera_cached_calcuations_check(&controller->_cache_calcluations, controller->camera);

    struct Vector3 offset;
    vector3Sub(&controller->look_target, &controller->player->transform.position, &offset);
    offset.y = 0.0f;

    float target_distance = sqrtf(vector3MagSqrd(&offset));

    float distance_ratio = target_distance * (1.0f / OVER_SHOULDER_DISTANCE);

    float sqrt_check = distance_ratio * distance_ratio - controller->_cache_calcluations.sin_1_3_fov_horz * controller->_cache_calcluations.sin_1_3_fov_horz;
    
    if (sqrt_check < 0.0f) {
        // TODO figure out
        return;
    }

    float inv_distance_ratio = 1.0f / distance_ratio;

    float sin_theta = (controller->_cache_calcluations.cos_1_3_fov_horz + sqrtf(sqrt_check)) * controller->_cache_calcluations.sin_1_3_fov_horz * inv_distance_ratio;
    float cos_theta = sqrtf(1.0f - sin_theta * sin_theta);

    vector3Scale(&offset, &offset, inv_distance_ratio);

    struct Vector3 target_check;
    target_check.x = offset.x * cos_theta - offset.z * sin_theta;
    target_check.y = controller->player->cutscene_actor.def->eye_level;
    target_check.z = offset.z * cos_theta + offset.x * sin_theta;

    vector3Add(&controller->player->transform.position, &target_check, result);

    target_check.x = offset.x * cos_theta + offset.z * sin_theta;
    target_check.y = controller->player->cutscene_actor.def->eye_level;
    target_check.z = offset.z * cos_theta - offset.x * sin_theta;
    vector3Add(&controller->player->transform.position, &target_check, &target_check);

    if (vector3DistSqrd(result, &controller->camera->transform.position) > 
        vector3DistSqrd(&target_check, &controller->camera->transform.position)) {
        *result = target_check;
    }
}

void camera_controller_determine_player_move_target(struct camera_controller* controller, struct Vector3* result, bool behind_player) {
    struct Vector3 offset;

    if (behind_player) {
        struct Quaternion quat;
        quatAxisComplex(&gUp, &controller->player->transform.rotation, &quat);
        quatMultVector(&quat, &gForward, &offset);
    } else {
        vector3Sub(&controller->player->transform.position, &controller->camera->transform.position, &offset);

        offset.y = 0.0f;
        vector3Normalize(&offset, &offset);

        if (vector3MagSqrd(&offset) < 0.1f) {
            offset = gForward;
        }
    }

    vector3AddScaled(&controller->player->transform.position, &offset, -CAMERA_FOLLOW_DISTANCE, result);
    result->y += CAMERA_FOLLOW_HEIGHT;
}

void camera_controller_update_position(struct camera_controller* controller, struct TransformSingleAxis* target) {
    move_towards(&controller->camera->transform.position, &controller->speed, &controller->target, &camera_move_parameters);

    struct Vector3 offset;
    vector3Sub(&controller->player->transform.position, &controller->camera->transform.position, &offset);

    vector3Sub(&target->position, &controller->camera->transform.position, &offset);
    offset.y += CAMERA_FOLLOW_HEIGHT;
    quatLook(&offset, &gUp, &controller->camera->transform.rotation);
}

void camera_controller_update(struct camera_controller* controller) {
    if (controller->state == CAMERA_STATE_FOLLOW) {
        camera_controller_determine_player_move_target(controller, &controller->target, joypad_get_buttons_held(0).z);
    } else if (controller->state == CAMERA_STATE_LOOK_AT_WITH_PLAYER) {
        camera_controller_determine_two_target_position(controller, &controller->target);
    }
    camera_controller_update_position(controller, &controller->player->transform);
}

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player) {
    controller->camera = camera;
    controller->player = player;

    update_add(controller, (update_callback)camera_controller_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    camera_controller_determine_player_move_target(controller, &controller->target, true);
    controller->follow_distace = 3.0f;

    controller->camera->transform.position = controller->target;
    controller->camera->transform.scale = gOneVec;
    controller->speed = 0.0f;
    controller->_cache_calcluations.fov = 0.0f;
    quatAxisAngle(&gRight, 0.0f, &controller->camera->transform.rotation);

    camera_controller_update_position(controller, &player->transform);
}

void camera_controller_destroy(struct camera_controller* controller) {
    update_remove(controller);
}

void camera_look_at(struct camera_controller* controller, struct Vector3* target) {
    controller->look_target = *target;
    controller->state = CAMERA_STATE_LOOK_AT_WITH_PLAYER;
}

void camera_follow_player(struct camera_controller* controller) {
    controller->state = CAMERA_STATE_FOLLOW;
}