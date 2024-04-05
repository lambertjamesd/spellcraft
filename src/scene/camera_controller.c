#include "camera_controller.h"

#include "../time/time.h"

#define CAMERA_FOLLOW_DISTANCE  3.4f
#define CAMERA_FOLLOW_HEIGHT    1.6f

void camera_controller_update_position(struct camera_controller* controller, struct Vector3* target) {
    struct Vector3 offset;
    
    vector3Sub(target, &controller->camera->transform.position, &offset);

    offset.y = 0.0f;
    vector3Normalize(&offset, &offset);

    if (vector3MagSqrd(&offset) < 0.1f) {
        offset = gForward;
    }

    struct Vector3 targetPosition;
    vector3AddScaled(target, &offset, -CAMERA_FOLLOW_DISTANCE, &targetPosition);
    targetPosition.y += CAMERA_FOLLOW_HEIGHT;

    vector3Lerp(&controller->camera->transform.position, &targetPosition, 0.1f, &controller->camera->transform.position);

    vector3Sub(target, &controller->camera->transform.position, &offset);
    offset.y += CAMERA_FOLLOW_HEIGHT;
    quatLook(&offset, &gUp, &controller->camera->transform.rotation);
}

void camera_controller_update(struct camera_controller* controller) {
    vector3Lerp(&controller->target, &controller->player->transform.position, 0.9f, &controller->target);

    camera_controller_update_position(controller, &controller->player->transform.position);
}

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player) {
    controller->camera = camera;
    controller->player = player;

    update_add(controller, (update_callback)camera_controller_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD);

    controller->target = player->transform.position;
    controller->follow_distace = 3.0f;

    controller->camera->transform.position = gZeroVec;
    controller->camera->transform.scale = gOneVec;
    quatAxisAngle(&gRight, 0.0f, &controller->camera->transform.rotation);

    camera_controller_update_position(controller, &player->transform.position);
}

void camera_controller_destroy(struct camera_controller* controller) {
    update_remove(controller);
}