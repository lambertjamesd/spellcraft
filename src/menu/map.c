#include "map.h"

#include "../resource/material_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"
#include "../menu/rsp_menu.h"
#include "./menu_common.h"
#include "../render/defs.h"

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
        0, 0, 100, 0,
        0, 0, 100, 0
    },
    .frac_part = {
        0xf700, 0, 0, 0,
        0, 0xf700, 0, 0
    },
};

static int offset_x = 5 << 2;
static int offset_y = 40 << 2;

void menu_map_render(menu_map_t* map, vector2s16_t* min, vector2s16_t* max) {
    menu_common_render_background(20, 20, 200, 200);

    material_apply(&map->outline_material->apply);
    
    int scroll = ((int)(total_time * 100.0f)) % 256;

    rdpq_set_tile_size_fx(
        TILE0, 
        scroll, 0, 
        scroll + map->outline_material->apply.tex0.s1, map->outline_material->apply.tex0.t1
    );
    
    menu_set_viewport(20, 20, 220, 220);
    rdpq_set_scissor(20, 20, 220, 220);
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&transform_test), true, true);

    for (int i = 0; i < map->room_count; i += 1) {
        rspq_block_run(map->rooms[i].outline.block);
    }
    
    menu_mtx_pop(1);

    float scale = powf(2.0f, fmodf(total_time / 8.0f, 2.0f));
    transform_2d_t transform = {
        scale, 0, 30,
        0, scale, 30
    };
    menu_transform_to_fixed(UncachedAddr(&transform_test), transform);
    
    rdpq_set_scissor(0, 0, SCREEN_WD, SCREEN_HT);

    // int16_t* tmp = menu_get_state();

    // for (int i = 0; i < 8; i += 1) {
    //     debugf("%04x ", tmp[i]);
    // }

    // debugf("\n");
    // for (int i = 8; i < 16; i += 1) {
    //     debugf("%04x ", tmp[i]);
    // }

    // debugf("\n\n");
}