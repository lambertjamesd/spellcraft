#include "camera_controller.h"

#include "../time/time.h"

void camera_controller_update_position(struct camera_controller* controller) {
    struct Vector3 offset;
    quatMultVector(&controller->camera->transform.rotation, &gForward, &offset);
    vector3AddScaled(&controller->target, &offset, controller->follow_distace, &controller->camera->transform.position);
    vector3AddScaled(&controller->camera->transform.position, &gUp, 1.0f, &controller->camera->transform.position);
}

void camera_controller_update(struct camera_controller* controller) {
    vector3Lerp(&controller->target, &controller->player->transform.position, 0.9f, &controller->target);

    camera_controller_update_position(controller);
}

void camera_controller_init(struct camera_controller* controller, struct Camera* camera, struct player* player) {
    controller->camera = camera;
    controller->player = player;

    update_add(controller, (update_callback)camera_controller_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD);

    controller->target = player->transform.position;
    controller->follow_distace = 6.0f;

    quatAxisAngle(&gRight, -3.14159f * 0.125f, &controller->camera->transform.rotation);

    camera_controller_update_position(controller);
}

void camera_controller_destroy(struct camera_controller* controller) {
    update_remove(controller);
}