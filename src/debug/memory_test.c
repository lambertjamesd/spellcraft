
#include "../test/framework_test.h"

#include "../resource/tmesh_cache.h"
#include "../resource/animation_cache.h"
#include "../resource/material_cache.h"
#include "../resource/sprite_cache.h"
#include "../resource/wav_cache.h"
#include "../scene/scene.h"
#include "../scene/scene_loader.h"
#include "../savefile/savefile.h"
#include "../entity/interactable.h"
#include "../util/cleanup.h"
#include "../menu/menu_common.h"
#include "../overworld/overworld_load.h"

heap_stats_t test_memory_leak_start(const char* name) {
    debugf("    memory leak test %s\n", name);

    heap_stats_t heap_stats;
    sys_get_heap_stats(&heap_stats);
    return heap_stats;
}

void test_memory_leak_end(struct test_context* t, heap_stats_t *start) {
    heap_stats_t heap_stats;
    sys_get_heap_stats(&heap_stats);
    test_eqi(t, start->used, heap_stats.used);
}

void test_resource_reset() {
    menu_common_destroy();
    interactable_reset();
    tmesh_cache_destroy();
    material_cache_destroy();
    sprite_cache_destroy();
    animation_cache_destroy();
    wav_cache_destroy();
}

void test_memory_leaks(struct test_context* t) {
    heap_stats_t heap_stats;

    tmesh_t player_mesh;
    heap_stats = test_memory_leak_start("tmesh_load_filename");
    tmesh_load_filename(&player_mesh, "rom:/meshes/characters/NPC_scrapbot1.tmesh");
    tmesh_release(&player_mesh);
    test_memory_leak_end(t, &heap_stats);

    tmesh_cache_destroy();
    heap_stats = test_memory_leak_start("tmesh_cache_load");
    tmesh_cache_release(tmesh_cache_load("rom:/meshes/characters/NPC_scrapbot1.tmesh"));
    tmesh_cache_destroy();
    test_memory_leak_end(t, &heap_stats);

    test_resource_reset();
    
    heap_stats = test_memory_leak_start("load_scene");
    cleanup_set_immediate(true);
    savefile_new();
    scene_t* scene = scene_load("rom:/scenes/inside_house.scene");
    scene_release(scene);
    savefile_unload();
    test_resource_reset();
    test_memory_leak_end(t, &heap_stats);


    heap_stats = test_memory_leak_start("load_overworld");
    cleanup_set_immediate(true);
    savefile_new();
    scene = scene_load("rom:/scenes/overworld.scene");

    for (int y = 0; y < 6; y += 1) {
        for (int x = 0; x < 6; x += 1) {
            scene->overworld->load_next.x = x;
            scene->overworld->load_next.y = y;
            overworld_check_loaded_tiles(scene->overworld);
            overworld_check_unload_queue(scene->overworld);
        }
    }

    vector3_t* player_pos = player_get_position(&scene->player);

    overworld_check_collider_tiles(scene->overworld, player_pos);
    player_pos->x += 500.0f;
    overworld_check_collider_tiles(scene->overworld, player_pos);

    scene_release(scene);
    savefile_unload();
    test_resource_reset();
    test_memory_leak_end(t, &heap_stats);

    cleanup_set_immediate(false);


}