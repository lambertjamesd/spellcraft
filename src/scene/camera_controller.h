#ifndef __SCENE_CAMERA_CONTROLLER_H__
#define __SCENE_CAMERA_CONTROLLER_H__

#include "../render/camera.h"
#include "player.h"

struct camera_controller {
    struct Camera* camera;
    struct player* player;
    int update_id;
    float follow_distace;
    struct Vector3 target;
};

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player);

void camera_controller_destroy(struct camera_controller* controller);

#endif