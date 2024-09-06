#include <libdragon.h>
#include <stdio.h>

#include "util/init.h"
#include "test/framework_test.h"

void test_mesh_index_lookup_triangle_indices(struct test_context* t);
void test_mesh_index_swept_lookup(struct test_context* t);
void test_collide_object_swept_to_triangle(struct test_context* t);
void test_collide_object_to_mesh_swept(struct test_context* t);
void test_collision_scene_collide_single(struct test_context* t);
void test_collision_scene_collide(struct test_context* t);
void test_ring_malloc(struct test_context* t);
void test_training_dummy(struct test_context* t);
void test_sweep_minkowski_sum(struct test_context* t);
void test_sweep_bounding_box(struct test_context* t);

#define DEBUG_CONNECT_DELAY     TICKS_FROM_MS(500)

int main() {
    rdpq_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();
    debug_init_isviewer();
    debug_init_usblog();
    
    // give time for the debugger to connect
    long long start_time = timer_ticks();
    while (timer_ticks() - start_time < DEBUG_CONNECT_DELAY);

    init_engine();

    console_init();
    console_set_render_mode(RENDER_MANUAL);

    test_run(test_mesh_index_lookup_triangle_indices);
    test_run(test_mesh_index_swept_lookup);

    test_run(test_collide_object_swept_to_triangle);
    test_run(test_collide_object_to_mesh_swept);

    test_run(test_collision_scene_collide_single);
    test_run(test_collision_scene_collide);

    test_run(test_ring_malloc);

    test_run(test_training_dummy);

    test_run(test_sweep_minkowski_sum);
    test_run(test_sweep_bounding_box);

    test_report_failures();

    return 0;
}