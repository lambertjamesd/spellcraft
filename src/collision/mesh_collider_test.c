#include "mesh_collider.h"
#include "../test/framework_test.h"
#include "collision_scene.h"

extern struct collision_scene g_scene;

struct test_collide_info {
    int collide_count;
};

void test_collide_info_init(struct test_collide_info* info) {
    info->collide_count = 0;
}

bool test_traingle_callback(struct mesh_index* index, void* data, int triangle_index) {
    struct test_collide_info* info = (struct test_collide_info*)data;

    info->collide_count += 1;

    return false;
}

void test_mesh_index_lookup_triangle_indices(struct test_context* t) {
    test_load_world("rom:/worlds/testing.world");

    struct test_collide_info info;
    test_collide_info_init(&info);

    mesh_index_lookup_triangle_indices(
        &g_scene.mesh_collider->index,
        &(struct Box3D){
            {10.0f, 40.0f, -10.0f},
            {30.0f, 45.0f, 10.0f},
        },
        test_traingle_callback,
        &info
    );

    test_equali(t, 2, info.collide_count);
}