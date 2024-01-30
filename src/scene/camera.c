#include "camera.h"

#include <GL/gl.h>

void cameraInit(struct Camera* camera, float fov, float near, float far) {
    transformInitIdentity(&camera->transform);
    camera->fov = fov;
    camera->near = near;
    camera->far = far;
}


void cameraApply(struct Camera* camera) {
    glMatrixMode(GL_PROJECTION);
    // glFrustum()

    glMatrixMode(GL_MODELVIEW);
    float mtx[4][4];

    struct Transform inverse;
    transformInvert(&camera->transform, &inverse);
    transformToMatrix(&inverse, mtx);
    glLoadMatrixf((GLfloat*)mtx);
}