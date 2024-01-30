#ifndef __SCENE_CAMERA_H__
#define __SCENE_CAMERA_H__

#include "../math/transform.h"

struct Camera {
    struct Transform transform;
    float fov;
    float near;
    float far;
};


void cameraInit(struct Camera* camera, float fov, float near, float far);

void cameraApply(struct Camera* camera);

#endif