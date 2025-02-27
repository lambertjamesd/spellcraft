#include "overworld.h"
#include "../test/framework_test.h"

int overworld_create_top_view(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector2* loop);

void test_overworld_create_top_view(struct test_context* t) {
    struct overworld overworld = {
        .min = {0.0f, 0.0f},
        .inv_tile_size = 1.0f,
    };

    mat4x4 projMatrix;
    matrixPerspective(projMatrix, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 4.0f);

    struct Vector2 loop[8];
    int loop_count = overworld_create_top_view(&overworld, projMatrix, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -4.0f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -4.0f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){1.0f, -1.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-1.0f, -1.0f}), &loop[3]);

    mat4x4 rotateMatrix;
    struct Quaternion rotation;
    quatAxisAngle(&gUp, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);

    mat4x4 combinedMatrix;
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){0.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){5.65685606f, 0.0f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){1.41421366f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){0.0f, -1.41421366f}), &loop[3]);

    quatAxisAngle(&gRight, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.65685606f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, 0.0f}), &loop[3]);

    quatAxisAngle(&gRight, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.65685606f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, 0.0f}), &loop[3]);

    quatAxisAngle(&gRight, 0.392699081, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, loop);
    test_eqi(t, 6, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.22625303f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.22625303f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -2.16478419f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){1.0f, -0.541196108f}), &loop[3]);
    test_vec2_equal(t, (&(struct Vector2){-1.0f, -0.541196108f}), &loop[4]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, -2.16478419f}), &loop[5]);
}