#include "map.h"

#include "../resource/material_cache.h"
#include "../resource/tmesh_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"

static tmesh_t* mesh_test;

#define MIN_SCALE   (1.0f / 16.0f)

void menu_map_load(menu_map_t* map, FILE* file) {
    fread(&map->room_count, sizeof(uint16_t), 1, file);
    fread(&map->min, sizeof(vector2_t), 1, file);
    fread(&map->max, sizeof(vector2_t), 1, file);

    map->rooms = map->room_count ? malloc(sizeof(menu_map_room_t) * map->room_count) : NULL;

    for (int i = 0; i < map->room_count; i += 1) {
        fread(&map->rooms[i].icon_count, 1, 1, file);
        tmesh_load(&map->rooms[i].outline, file);
    }

    map->outline_material = material_cache_load("rom:/materials/menu/map_mesh.mat");

    mesh_test = tmesh_cache_load("rom:/meshes/test/sphere_test.tmesh");
}

void menu_map_destroy(menu_map_t* map) {
    for (int i = 0; i < map->room_count; i += 1) {
        tmesh_release(&map->rooms[i].outline);
    }
    free(map->rooms);

    material_cache_release(map->outline_material);
}

void menu_map_render(menu_map_t* map, vector2s16_t* min, vector2s16_t* max) {
    material_apply(&map->outline_material->apply);
    
    T3DMat4FP* mtx = frame_pool_get_transformfp(frame_pool_curr());

    T3DMat4 mat;
    t3d_mat4_identity(&mat);
    t3d_mat4_scale(&mat, MIN_SCALE, MIN_SCALE, MIN_SCALE);
    t3d_mat4_translate(&mat, 40, 40, 0);
    t3d_mat4_to_fixed_3x4(mtx, &mat);
    t3d_matrix_push(mtx);

    int scroll = ((int)(game_time * 100.0f)) % 256;

    rdpq_set_tile_size_fx(
        TILE0, 
        scroll, 0, 
        scroll + map->outline_material->apply.tex0.s1, map->outline_material->apply.tex0.t1
    );

    rdpq_set_prim_register_raw((color_t){
        .r = 255,
        .g = 255,
        .b = 255,
        .a = 64,
    }, 0, 0);

    for (int i = 0; i < map->room_count; i += 1) {
        rspq_block_run(map->rooms[i].outline.block);
    }

    t3d_matrix_pop(1);
}