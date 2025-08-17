#include "camera_controller.h"

#include "../time/time.h"
#include "../math/transform_single_axis.h"
#include "../physics/move_towards.h"
#include "../math/mathf.h"
#include "../math/constants.h"
#include <math.h>

#define CAMERA_FOLLOW_DISTANCE  3.4f
#define CAMERA_FOLLOW_HEIGHT    1.6f

static struct move_towards_parameters camera_move_parameters = {
    .max_speed = 30.0f,
    .max_accel = 10.0f,
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
    vector3Sub(&controller->look_target, player_get_position(controller->player), &offset);
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

    vector3Add(player_get_position(controller->player), &target_check, result);

    target_check.x = offset.x * cos_theta + offset.z * sin_theta;
    target_check.y = controller->player->cutscene_actor.def->eye_level;
    target_check.z = offset.z * cos_theta - offset.x * sin_theta;
    vector3Add(player_get_position(controller->player), &target_check, &target_check);

    if (vector3DistSqrd(result, &controller->camera->transform.position) > 
        vector3DistSqrd(&target_check, &controller->camera->transform.position)) {
        *result = target_check;
    }
}

void camera_controller_determine_player_move_target(struct camera_controller* controller, struct Vector3* result, bool behind_player) {
    struct Vector3 offset;

    if (behind_player) {
        struct Quaternion quat;
        quatAxisComplex(&gUp, &controller->player->cutscene_actor.transform.rotation, &quat);
        quatMultVector(&quat, &gForward, &offset);
    } else {
        vector3Sub(player_get_position(controller->player), &controller->camera->transform.position, &offset);

        offset.y = 0.0f;
        vector3Normalize(&offset, &offset);

        if (vector3MagSqrd(&offset) < 0.1f) {
            offset = gForward;
        }
    }

    vector3AddScaled(player_get_position(controller->player), &offset, -CAMERA_FOLLOW_DISTANCE, result);
    result->y += CAMERA_FOLLOW_HEIGHT;
}

void camera_controller_update_position(struct camera_controller* controller, struct TransformSingleAxis* target) {
    move_towards(&controller->camera->transform.position, &controller->speed, &controller->target, &camera_move_parameters);

    struct Vector3 offset;
    vector3Sub(player_get_position(controller->player), &controller->camera->transform.position, &offset);

    vector3Sub(&target->position, &controller->camera->transform.position, &offset);
    offset.y += CAMERA_FOLLOW_HEIGHT;
    quatLook(&offset, &gUp, &controller->camera->transform.rotation);
}

void camera_controller_return_target(struct camera_controller* controller, struct Vector3* target) {
    struct Vector3 offset;
    quatMultVector(&controller->camera->transform.rotation, &gForward, &offset);
    offset.y = 0.0f;
    vector3Scale(&offset, &offset, -1.0f);
    vector3Normalize(&offset, &offset);
    vector3AddScaled(player_get_position(controller->player), &offset, -CAMERA_FOLLOW_DISTANCE, target);
    target->y += CAMERA_FOLLOW_HEIGHT;

    move_towards(&controller->camera->transform.position, &controller->speed, &controller->target, &camera_move_parameters);

    controller->camera->fov = mathfMoveTowards(controller->camera->fov, 70.0f, 20.0f * fixed_time_step);

    if (vector3DistSqrd(target, &controller->camera->transform.position) < 0.0001f) {
        controller->state = CAMERA_STATE_FOLLOW;
        controller->camera->fov = 70.0f;
    }
}

static struct camera_animation_frame __attribute__((aligned(16))) anim_frame_buffer;

void camera_controller_update_animation(struct camera_controller* controller) {
    if (!controller->animation || controller->current_frame >= controller->animation->frame_count) {
        return;
    }

    data_cache_hit_invalidate((void*)&anim_frame_buffer, 16);
    dma_read(
        &anim_frame_buffer, 
        controller->animation->rom_offset + sizeof(struct camera_animation_frame) * controller->current_frame, 
        sizeof(struct camera_animation_frame)
    );

    controller->camera->transform.position = anim_frame_buffer.position;
    controller->camera->transform.rotation.x = anim_frame_buffer.rotation[0];
    controller->camera->transform.rotation.y = anim_frame_buffer.rotation[1];
    controller->camera->transform.rotation.z = anim_frame_buffer.rotation[2];

    float neg_w = controller->camera->transform.rotation.x * controller->camera->transform.rotation.x 
        + controller->camera->transform.rotation.y * controller->camera->transform.rotation.y 
        + controller->camera->transform.rotation.z * controller->camera->transform.rotation.z;

    if (neg_w > 1.0f) {
        neg_w = 1.0f;
    }

    controller->camera->transform.rotation.w = sqrtf(1.0f - neg_w);
    controller->camera->fov = (180.0f / M_PI) * anim_frame_buffer.fov;
    controller->current_frame += 1;
}

void camera_controller_update(struct camera_controller* controller) {
    if (controller->state == CAMERA_STATE_FOLLOW) {
        camera_controller_determine_player_move_target(controller, &controller->target, joypad_get_buttons_held(0).z);
    } else if (controller->state == CAMERA_STATE_LOOK_AT_WITH_PLAYER) {
        camera_controller_determine_two_target_position(controller, &controller->target);
    } else if (controller->state == CAMERA_STATE_ANIMATE) {
        camera_controller_update_animation(controller);
        return;
    } else if (controller->state == CAMERA_STATE_RETURN_TO_PLAYER) {
        camera_controller_return_target(controller, &controller->target);
        return;
    } else if (controller->state == CAMERA_STATE_FIXED) {
        return;
    }
    camera_controller_update_position(controller, &controller->player->cutscene_actor.transform);
}

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player) {
    controller->camera = camera;
    controller->player = player;
    controller->state = CAMERA_STATE_FOLLOW;

    update_add(controller, (update_callback)camera_controller_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    camera_controller_determine_player_move_target(controller, &controller->target, true);
    controller->follow_distace = 3.0f;

    controller->camera->transform.position = controller->target;
    controller->camera->transform.scale = gOneVec;
    controller->speed = 0.0f;
    controller->_cache_calcluations.fov = 0.0f;
    quatAxisAngle(&gRight, 0.0f, &controller->camera->transform.rotation);

    camera_controller_update_position(controller, &player->cutscene_actor.transform);

    controller->animation = NULL;
    controller->current_frame = 0;
}

void camera_controller_destroy(struct camera_controller* controller) {
    update_remove(controller);
}

void camera_look_at(struct camera_controller* controller, struct Vector3* target) {
    controller->look_target = *target;
    controller->state = CAMERA_STATE_LOOK_AT_WITH_PLAYER;
    controller->camera->fov = 70.0f;
}

void camera_follow_player(struct camera_controller* controller) {
    controller->state = CAMERA_STATE_FOLLOW;
    controller->camera->fov = 70.0f;
}

void camera_return(struct camera_controller* controller) {
    controller->state = CAMERA_STATE_RETURN_TO_PLAYER;
    controller->speed = 0.0f;
}

void camera_play_animation(struct camera_controller* controller, struct camera_animation* animation) {
    controller->state = CAMERA_STATE_ANIMATE;
    controller->animation = animation;
    controller->current_frame = 0;
}

void camera_set_fixed(struct camera_controller* controller, struct Vector3* position, struct Quaternion* rotation, float fov) {
    controller->state = CAMERA_STATE_FIXED;
    controller->camera->transform.position = *position;
    controller->camera->transform.rotation = *rotation;
    controller->camera->fov = fov;
}

bool camera_is_animating(struct camera_controller* controller) {
    return controller->animation != 0 && controller->current_frame < controller->animation->frame_count;
}