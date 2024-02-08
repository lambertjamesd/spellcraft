#include "camera.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include <math.h>

#include "../math/matrix.h"

void camera_init(struct Camera* camera, float fov, float near, float far) {
    transformInitIdentity(&camera->transform);
    camera->fov = fov;
    camera->near = near;
    camera->far = far;
}


void camera_extract_clipping_plane(float viewPersp[4][4], struct Plane* output, int axis, float direction) {
    output->normal.x = viewPersp[0][axis] * direction + viewPersp[0][3];
    output->normal.y = viewPersp[1][axis] * direction + viewPersp[1][3];
    output->normal.z = viewPersp[2][axis] * direction + viewPersp[2][3];
    output->d = viewPersp[3][axis] * direction + viewPersp[3][3];

    float mult = 1.0f / sqrtf(vector3MagSqrd(&output->normal));
    vector3Scale(&output->normal, &output->normal, mult);
    output->d *= mult;
}


void camera_apply(struct Camera* camera, float aspect_ratio, struct ClippingPlanes* clipping_planes) {
    glMatrixMode(GL_PROJECTION);

    float tan_fov = tanf(camera->fov * (0.5f * 3.14159f / 180.0f));

    mat4x4 proj_matrix;
    matrixPerspective(
        proj_matrix, 
        -aspect_ratio * tan_fov * camera->near,
        aspect_ratio * tan_fov * camera->near,
        tan_fov * camera->near,
        -tan_fov * camera->near,
        camera->near,
        camera->far
    );

    glLoadMatrixf((GLfloat*)proj_matrix);

    glMatrixMode(GL_MODELVIEW);
    mat4x4 view_matrix;

    struct Transform inverse;
    transformInvert(&camera->transform, &inverse);
    transformToMatrix(&inverse, view_matrix);
    glLoadMatrixf((GLfloat*)view_matrix);

    if (!clipping_planes) {
        return;
    }

    mat4x4 view_proj_matrix;
    matrixMul(proj_matrix, view_matrix, view_proj_matrix);

    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[0], 0, 1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[1], 0, -1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[2], 1, 1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[3], 1, -1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[4], 2, 1.0f);
}