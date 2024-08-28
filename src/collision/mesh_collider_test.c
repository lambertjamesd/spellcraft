#include "mesh_collider.h"
#include "../test/framework_test.h"
#include "collision_scene.h"

extern struct collision_scene g_scene;

struct test_collide_info {
    int collide_count;
    uint64_t colliders;
    bool should_collide;
};

void test_collide_info_init(struct test_collide_info* info) {
    info->collide_count = 0;
    info->colliders = 0;
    info->should_collide = true;
}

bool test_traingle_callback(struct mesh_index* index, void* data, int triangle_index) {
    struct test_collide_info* info = (struct test_collide_info*)data;

    info->collide_count += 1;
    info->colliders |= (1 << triangle_index);

    return info->should_collide;
}

void test_mesh_index_lookup_triangle_indices(struct test_context* t) {
    test_load_scene("rom:/scenes/testing.scene");

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

    test_eqi(t, 2, info.collide_count);
}

void test_mesh_index_swept_lookup(struct test_context* t) {
    test_load_scene("rom:/scenes/testing.scene");

    struct test_collide_info info;
    test_collide_info_init(&info);

    mesh_index_swept_lookup(
        &g_scene.mesh_collider->index, 
        &(struct Box3D) {
            {10.0f, 40.0f, -10.0f},
            {30.0f, 45.0f, 10.0f},
        },
        &(struct Vector3){40.0f, 0.f, 0.0f},
        test_traingle_callback,
        &info
    );

    test_eqi(t, 2, info.collide_count);
    
    struct test_collide_info lower_info;
    test_collide_info_init(&lower_info);

    mesh_index_swept_lookup(
        &g_scene.mesh_collider->index, 
        &(struct Box3D) {
            {10.0f, 4.0f, -4.0f},
            {30.0f, 6.0f, 4.0f},
        },
        &(struct Vector3){40.0f, 0.f, 0.0f},
        test_traingle_callback,
        &lower_info
    );

    test_eqi(t, 6, lower_info.collide_count);
    test_eqi(t, 0, info.colliders & lower_info.colliders);

    test_collide_info_init(&lower_info);
    lower_info.should_collide = false;

    mesh_index_swept_lookup(
        &g_scene.mesh_collider->index, 
        &(struct Box3D) {
            {10.0f, 4.0f, -4.0f},
            {30.0f, 6.0f, 4.0f},
        },
        &(struct Vector3){40.0f, 0.f, 0.0f},
        test_traingle_callback,
        &lower_info
    );

    test_eqi(t, 13, lower_info.collide_count);
    test_eqi(t, 0, !(info.colliders & lower_info.colliders));
}