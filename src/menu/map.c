#include "map.h"

#include "../resource/material_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"
#include "../menu/rsp_menu.h"

static rspq_block_t* block_test;

#define MIN_SCALE   (1.0f / 16.0f)

void menu_map_load(menu_map_t* map, FILE* file) {
    fread(&map->room_count, sizeof(uint16_t), 1, file);
    fread(&map->min, sizeof(vector2_t), 1, file);
    fread(&map->max, sizeof(vector2_t), 1, file);

    map->rooms = map->room_count ? malloc(sizeof(menu_map_room_t) * map->room_count) : NULL;

    for (int i = 0; i < map->room_count; i += 1) {
        fread(&map->rooms[i].icon_count, 1, 1, file);
        mesh2d_load(&map->rooms[i].outline, file);
    }

    map->outline_material = material_cache_load("rom:/materials/menu/map_mesh.mat");

    rspq_block_begin();
    rdpq_fill_rectangle(10, 10, 20, 20);
    block_test = rspq_block_end();
}

void menu_map_destroy(menu_map_t* map) {
    for (int i = 0; i < map->room_count; i += 1) {
        mesh2d_release(&map->rooms[i].outline);
    }
    free(map->rooms);

    material_cache_release(map->outline_material);
}

static transform_2d_fp_t transform_test = {
    .int_part = {
        1, 0, 40, 0,
        0, 1, 40, 0
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
    
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&transform_test), true, true);

    for (int i = 0; i < map->room_count; i += 1) {
        rspq_block_run(map->rooms[i].outline.block);
    }
    
    menu_mtx_pop(1);
}