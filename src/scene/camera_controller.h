#ifndef __SCENE_CAMERA_CONTROLLER_H__
#define __SCENE_CAMERA_CONTROLLER_H__

#include "../render/camera.h"
#include "../player/player.h"
#include "camera_animation.h"

enum camera_controller_state {
    CAMERA_STATE_FOLLOW,
    CAMERA_STATE_LOOK_AT_WITH_PLAYER,
    CAMERA_STATE_ANIMATE,
};

struct camera_cached_calcuations {
    float fov;
    float fov_horz;
    float cos_1_3_fov_horz;
    float sin_1_3_fov_horz;
};

struct camera_controller {
    struct Camera* camera;
    struct player* player;
    struct camera_cached_calcuations _cache_calcluations;
    float follow_distace;
    struct Vector3 target;
    float speed;
    struct Vector3 look_target;
    enum camera_controller_state state;
    struct camera_animation* animation;
    uint16_t current_frame;
};

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player);

void camera_controller_destroy(struct camera_controller* controller);

void camera_look_at(struct camera_controller* controller, struct Vector3* target);
void camera_follow_player(struct camera_controller* controller);
void camera_play_animation(struct camera_controller* controller, struct camera_animation* animation);

bool camera_is_animating(struct camera_controller* controller);

#endif