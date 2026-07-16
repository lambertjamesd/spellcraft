#include "map.h"

#include "../resource/material_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"
#include "../menu/rsp_menu.h"
#include "./menu_common.h"
#include "../render/defs.h"

#define MIN_SCALE       (1.0f / 16.0f)
#define HEADER_FOOTER   0x4D415020

void menu_map_load_layer(menu_map_layer_t* layer, menu_map_data_t* data, FILE* file) {
    fread(&layer->room_count, sizeof(uint16_t), 1, file);
    layer->rooms = data->layer_rooms;
    data->layer_rooms += layer->room_count;

    for (int i = 0; i < layer->room_count; i += 1) {
        menu_map_layer_room_t* room = &layer->rooms[i];
        fread(&room->room_index, sizeof(uint16_t), 1, file);
        fread(&room->icon_count, sizeof(uint16_t), 1, file);

        room->icons = NULL;
        mesh2d_load(&room->outline, file);
    }
}

void menu_map_room(menu_map_room_t* layer, menu_map_data_t* data, FILE* file) {
    fread(&layer->min_layer, sizeof(uint16_t), 1, file);
    fread(&layer->layer_count, sizeof(uint16_t), 1, file);
    layer->max_layer_y = data->room_layer_y;
    data->room_layer_y += layer->layer_count;

    fread(layer->max_layer_y, sizeof(float), layer->layer_count, file);
}

void menu_map_load(menu_map_t* map, FILE* file) {
    int header_footer;
    fread(&header_footer, sizeof(int), 1, file);
    assert(header_footer == HEADER_FOOTER);

    fread(&map->layer_count, sizeof(uint16_t), 1, file);
    fread(&map->room_count, sizeof(uint16_t), 1, file);

    fread(&map->layer_room_count, sizeof(uint16_t), 1, file);
    fread(&map->room_layer_y_count, sizeof(uint16_t), 1, file);

    fread(&map->min, sizeof(vector2_t), 1, file);
    fread(&map->max, sizeof(vector2_t), 1, file);

    map->layers = map->layer_count ? malloc(sizeof(menu_map_layer_t) * map->layer_count) : NULL;
    map->rooms = map->room_count ? malloc(sizeof(menu_map_room_t) * map->room_count) : NULL;

    map->data.layer_rooms = map->layer_room_count ? malloc(sizeof(menu_map_layer_room_t) * map->layer_room_count) : NULL;
    map->data.room_layer_y = map->room_layer_y_count ? malloc(sizeof(float) * map->room_layer_y_count) : NULL;

    menu_map_data_t data = map->data;

    for (int i = 0; i < map->layer_count; i += 1) {
        menu_map_load_layer(&map->layers[i], &data, file);
    }

    for (int i = 0; i < map->room_count; i += 1) {
        menu_map_room(&map->rooms[i], &data, file);
    }

    map->outline_material = material_cache_load("rom:/materials/menu/map_mesh.mat");
    
    fread(&header_footer, sizeof(int), 1, file);
    assert(header_footer == HEADER_FOOTER);
}

void menu_map_destroy(menu_map_t* map) {
    for (int i = 0; i < map->layer_room_count; i += 1) {
        mesh2d_release(&map->data.layer_rooms[i].outline);
    }
    free(map->layers);
    free(map->rooms);
    free(map->data.layer_rooms);
    free(map->data.room_layer_y);

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

static transform_2d_fp_t uv_transform_test = {
    .int_part = {
        0, 1, 0, 0,
        -1, 0, 0, 0
    },
    .frac_part = {
        0, 0, 0, 0,
        0, 0, 0, 0
    },
};

static int offset_x = 5 << 2;
static int offset_y = 40 << 2;

void menu_map_render(menu_map_t* map, vector2s16_t* min, vector2s16_t* max) {
    menu_common_render_background(20, 20, 200, 200);

    material_apply(&map->outline_material->apply);
    
    int scroll = (((int)(total_time * 200.0f)) % (32 * 32));

    rdpq_set_tile_size_fx(
        TILE0, 
        scroll, 0, 
        scroll + map->outline_material->apply.tex0.s1, map->outline_material->apply.tex0.t1
    );
    
    menu_set_viewport(20, 20, 220, 220);
    rdpq_set_scissor(20, 20, 220, 220);
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&transform_test), true, true);
    menu_mtx_uv((transform_2d_fp_t*)PhysicalAddr(&uv_transform_test), true, true);

    for (int i = 0; i < map->layer_room_count; i += 1) {
        rspq_block_run(map->data.layer_rooms[i].outline.block);
    }
    
    menu_mtx_pop(1);
    menu_mtx_pop_uv(1);
    
    rdpq_set_scissor(0, 0, SCREEN_WD, SCREEN_HT);

    // uint8_t* tmp = menu_get_state();

    // int16_t* tri_data = (int16_t*)(tmp + RSP_MENU_RDPQ_TRI_DATA);

    // debugf("%d %d\n", ((int*)tmp)[0], *tri_data);

    // for (int i = 0; i < 8; i += 1) {
    //     debugf("%04x ", tmp[i]);
    // }

    // debugf("\n");
    // for (int i = 8; i < 16; i += 1) {
    //     debugf("%04x ", tmp[i]);
    // }

    // debugf("\n\n");
}