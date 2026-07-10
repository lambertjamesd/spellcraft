#include "map.h"

#include "../resource/material_cache.h"
#include "../resource/tmesh_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"
#include "../menu/rsp_menu.h"

static tmesh_t* mesh_test;
static rspq_block_t* block_test;

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

    rspq_block_begin();
    rdpq_fill_rectangle(10, 10, 20, 20);
    block_test = rspq_block_end();
}

void menu_map_destroy(menu_map_t* map) {
    for (int i = 0; i < map->room_count; i += 1) {
        tmesh_release(&map->rooms[i].outline);
    }
    free(map->rooms);

    material_cache_release(map->outline_material);
}

static transform_2d_fp_t transform_test = {
    .int_part = {
        2, 0, 0, 0,
        0, 2, 0, 0
    },
    .frac_part = {
        0, 0, 0, 0,
        0, 0, 0, 0
    },
};

static int offset_x = 5 << 2;
static int offset_y = 40 << 2;

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

    // rspq_block_run(block_test);

    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&transform_test), true, true);

    int offset = (int)(game_time * 16.0f) % 160 + 1;

    menu_set_attr_flags(MENU_FLAGS_SHADE);
    menu_move_to(&(vector2s16_t){
        .x = 50 << 2,
        .y = 10 << 2,
    }, 0, 10 << 2, (color_t){
        255, 0, 0, 255
    });

    menu_line_to(&(vector2s16_t){
        .x = (50 << 2) + offset,
        .y = (10 << 2) + offset_y,
    }, 0, 10 << 2, (color_t){
        0, 255, 0, 255
    });

    rspq_wait();

    short* test = menu_get_state();

    for (int i = 0; i < 16; i += 1) {
        debugf("%d ", (int)test[i]);

        if (i == 7) {
            debugf("\n");
        }
    }
    debugf("\n\n");
    
    menu_mtx_pop(1);
}