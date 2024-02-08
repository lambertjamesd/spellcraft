#ifndef __SCENE_CAMERA_H__
#define __SCENE_CAMERA_H__

#include "../math/transform.h"
#include "../math/plane.h"

struct ClippingPlanes {
    struct Plane planes[5];
};

struct Camera {
    struct Transform transform;
    float fov;
    float near;
    float far;
};

void camera_init(struct Camera* camera, float fov, float near, float far);

void camera_apply(struct Camera* camera, float aspect_ratio, struct ClippingPlanes* clipping_planes);

#endif