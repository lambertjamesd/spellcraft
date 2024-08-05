#include "collide_swept.h"
#include "../test/framework_test.h"
#include <stddef.h>

void object_mesh_collide_data_init(
    struct object_mesh_collide_data* data,
    struct Vector3* prev_pos,
    struct mesh_collider* mesh,
    struct dynamic_object* object
);

bool collide_object_swept_to_triangle(struct mesh_index* index, void* data, int triangle_index);

static struct mesh_collider single_traingle_mesh = {
    .vertices = (struct Vector3[]){
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    },
    .triangles = (struct mesh_triangle_indices[]){
        {{0, 1, 2}},
    },
    .triangle_count = 1,
    .index = {
        .min = {0.0f, 0.0f, 0.0f},
        .stride_inv = {1.0f, 1.0f, 1.0f},
        .block_count = {1, 1, 1},
        .blocks = (struct mesh_index_block[]){{0, 1}},
        .index_indices = (uint16_t[]){0},
    },
};

static struct dynamic_object_type simple_cube_object = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = dynamic_object_box_bouding_box,
    .data = {
        .box = {
            .half_size = {0.05f, 0.05f, 0.05f},
        }
    }
};

void test_collide_object_swept_to_triangle(struct test_context* t) {
    struct object_mesh_collide_data collide_data;
    struct Vector3 prev_pos = {0.25f, 0.25f, -1.0f};
    struct Vector3 position = {0.25f, 0.25f, 1.0f};

    struct dynamic_object object;
    dynamic_object_init(1, &object, &simple_cube_object, ~0, &position, NULL);

    object_mesh_collide_data_init(&collide_data, &prev_pos, &single_traingle_mesh, &object);

    bool did_hit = collide_object_swept_to_triangle(&single_traingle_mesh.index, &collide_data, 0);

    test_equali(t, true, did_hit);
}