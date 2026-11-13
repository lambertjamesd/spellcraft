#include "../test/framework_test.h"

#include "../resource/tmesh_cache.h"
#include "../resource/resource_cache.h"
#include "../scene/scene.h"
#include "../scene/scene_loader.h"
#include "../util/cleanup.h"
#include "../overworld/overworld_load.h"
#include "../overworld/overworld_private.h"

typedef void (*memory_test_scenario)();

static void test_flush_deferred_frees() {
    // resource caches and cleanup_safe free resources once the RSP is idle
    resource_cache_flush();
}

// runs the scenario once to warm up any lazily allocated memory,
// such as resource cache tables, then checks a second run for leaks
static void test_check_for_leak(struct test_context* t, const char* name, memory_test_scenario scenario) {
    debugf("    memory leak test %s\n", name);

    scenario();
    test_flush_deferred_frees();

    heap_stats_t start;
    sys_get_heap_stats(&start);

    scenario();
    test_flush_deferred_frees();

    heap_stats_t end;
    sys_get_heap_stats(&end);

    if (start.used != end.used) {
        debugf("    leaked %d bytes in %s\n", end.used - start.used, name);
    }

    test_eqi(t, start.used, end.used);
}

static void scenario_tmesh_load_filename() {
    tmesh_t mesh;
    tmesh_load_filename(&mesh, "rom:/meshes/characters/apprentice.tmesh");
    tmesh_release(&mesh);
}

static void scenario_tmesh_cache_load() {
    // not one of the globally loaded assets so it is freed on release
    tmesh_cache_release(tmesh_cache_load("rom:/meshes/puzzle/pulley_gate.tmesh"));
}

static scene_t* load_scene(const char* name) {
    scene_queue_next(name);
    scene_t* result = scene_load(scene_get_next());
    scene_clear_next();
    return result;
}

static void scenario_load_scene() {
    scene_release(load_scene("rom:/scenes/testing.scene"));
}

static void scenario_load_overworld() {
    scene_t* scene = load_scene("rom:/scenes/overworld_test.scene");
    struct overworld* overworld = scene->overworld;

    for (int y = 0; y < 6 && y < overworld->tile_y; y += 1) {
        for (int x = 0; x < 6 && x < overworld->tile_x; x += 1) {
            overworld->load_next.x = x;
            overworld->load_next.y = y;
            overworld_check_loaded_tiles(overworld);
            overworld_check_unload_queue(overworld);
        }
    }

    vector3_t* player_pos = player_get_position(&scene->player);

    overworld_check_collider_tiles(overworld, player_pos);
    player_pos->x += 500.0f;
    overworld_check_collider_tiles(overworld, player_pos);

    scene_release(scene);
}

void test_memory_leaks(struct test_context* t) {
    // make sure scenes from other tests don't interfere
    test_unload_scene();
    test_flush_deferred_frees();

    test_check_for_leak(t, "tmesh_load_filename", scenario_tmesh_load_filename);
    test_check_for_leak(t, "tmesh_cache_load", scenario_tmesh_cache_load);

    cleanup_set_immediate(true);
    test_check_for_leak(t, "load_scene", scenario_load_scene);
    test_check_for_leak(t, "load_overworld", scenario_load_overworld);
    cleanup_set_immediate(false);
}
