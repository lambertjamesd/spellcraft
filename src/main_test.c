#include <libdragon.h>
#include <stdio.h>

#include "util/init.h"
#include "test/framework_test.h"

void test_mesh_index_lookup_triangle_indices(struct test_context* t);
void test_mesh_index_swept_lookup(struct test_context* t);
void test_collide_object_swept_to_triangle(struct test_context* t);

#define DEBUG_CONNECT_DELAY     TICKS_FROM_MS(500)

int main() {
    rdpq_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();
    debug_init_isviewer();
    debug_init_usblog();
    
    long long start_time = timer_ticks();

    while (timer_ticks() - start_time < DEBUG_CONNECT_DELAY);

    init_engine();

    console_init();
    console_set_render_mode(RENDER_MANUAL);

    test_run(test_mesh_index_lookup_triangle_indices);
    test_run(test_mesh_index_swept_lookup);

    test_run(test_collide_object_swept_to_triangle);

    test_report_failures();

    return 0;
}