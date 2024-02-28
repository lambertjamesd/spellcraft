#ifndef __RENDER_CAMERA_H__
#define __RENDER_CAMERA_H__

#include "../math/transform.h"
#include "../math/plane.h"
#include "../math/matrix.h"

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

void camera_apply(struct Camera* camera, float aspect_ratio, struct ClippingPlanes* clipping_planes, mat4x4 view_proj_matrix);

#endif