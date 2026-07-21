#include "map.h"

#include "../resource/material_cache.h"
#include "../render/frame_alloc.h"
#include "../time/time.h"
#include "../menu/rsp_menu.h"
#include "./menu_common.h"
#include "../render/defs.h"
#include "../render/frame_alloc.h"
#include "../scene/scene.h"
#include "../font/fonts.h"
#include "../cutscene/expression_evaluate.h"

#include "relative_text.h"

#define MAP_SCALE       4 * 200.0f
#define MIN_SCALE       (1.0f / 16.0f)
#define HEADER_FOOTER   0x4D415020

static rdpq_paragraph_t* paragraph_test;

void menu_map_load_layer(menu_map_layer_t* layer, menu_map_data_t* data, FILE* file) {
    fread(&layer->room_count, sizeof(uint16_t), 1, file);
    layer->rooms = data->layer_rooms;
    data->layer_rooms += layer->room_count;

    for (int i = 0; i < layer->room_count; i += 1) {
        menu_map_layer_room_t* room = &layer->rooms[i];
        fread(&room->center, sizeof(vector2s16_t), 1, file);
        fread(&room->room_index, sizeof(uint16_t), 1, file);
        fread(&room->icon_count, sizeof(uint16_t), 1, file);

        room->icons = data->icons;
        data->icons += room->icon_count;

        menu_map_icon_t* icon = room->icons;

        for (int icon_index = 0; icon_index < room->icon_count; icon_index += 1) {
            fread(&icon->pos, sizeof(vector2s16_t), 1, file);
            fread(&icon->hidden, sizeof(uint16_t), 1, file);
            fread(&icon->icon_type, sizeof(uint8_t), 1, file);
            icon += 1;
        }

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
    fread(&map->icon_count, sizeof(uint16_t), 1, file);

    fread(&map->min, sizeof(vector2_t), 1, file);
    fread(&map->max, sizeof(vector2_t), 1, file);

    map->size_inv = MAP_SCALE / (map->max.x - map->min.x);

    map->layers = map->layer_count ? malloc(sizeof(menu_map_layer_t) * map->layer_count) : NULL;
    map->rooms = map->room_count ? malloc(sizeof(menu_map_room_t) * map->room_count) : NULL;
    
    map->data.layer_rooms = map->layer_room_count ? malloc(sizeof(menu_map_layer_room_t) * map->layer_room_count) : NULL;
    map->data.room_layer_y = map->room_layer_y_count ? malloc(sizeof(float) * map->room_layer_y_count) : NULL;
    map->data.icons = map->icon_count ? malloc(sizeof(menu_map_icon_t) * map->icon_count) : NULL;

    menu_map_data_t data = map->data;

    for (int i = 0; i < map->layer_count; i += 1) {
        menu_map_load_layer(&map->layers[i], &data, file);
    }

    for (int i = 0; i < map->room_count; i += 1) {
        menu_map_room(&map->rooms[i], &data, file);
    }

    map->outline_material = material_cache_load("rom:/materials/menu/map_mesh.mat");
    map->solid_color = material_cache_load("rom:/materials/menu/solid_primitive.mat");
    map->map_icon_material = material_cache_load("rom:/materials/menu/map_icons.mat");
    map->number_font_material = material_cache_load("rom:/materials/menu/number_font.mat");
    
    fread(&header_footer, sizeof(int), 1, file);
    assert(header_footer == HEADER_FOOTER);

    int nbytes = 3;
    paragraph_test = rdpq_paragraph_build(&(rdpq_textparms_t){}, FONT_DIALOG, "0/1", &nbytes);
}

void menu_map_destroy(menu_map_t* map) {
    for (int i = 0; i < map->layer_room_count; i += 1) {
        mesh2d_release(&map->data.layer_rooms[i].outline);
    }
    free(map->layers);
    free(map->rooms);
    free(map->data.layer_rooms);
    free(map->data.room_layer_y);
    free(map->data.icons);

    material_cache_release(map->outline_material);
    material_cache_release(map->solid_color);
    material_cache_release(map->map_icon_material);
    material_cache_release(map->number_font_material);

    rdpq_paragraph_free(paragraph_test);
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

static menu2d_vtx_t player_arrow[] = {
    {.pos = {{{0.0f, -20.0f}}}, .color = {0xFF, 0xFF, 0xFF, 0xFF}},
    {.pos = {{{10.0f, 20.0f}}}, .color = {0xFF, 0xFF, 0xFF, 0xFF}},
    {.pos = {{{-10.0f, 20.0f}}}, .color = {0xFF, 0xFF, 0xFF, 0xFF}},
};

void menu_map_render_player(menu_map_t* map) {
    vector3_t* player_pos = player_get_position(&current_scene->player);
    vector2_t* player_rot = player_get_rotation(&current_scene->player);

    transform_2d_fp_t* mtx = frame_malloc(frame_pool_curr(), sizeof(transform_2d_fp_t));
    transform_2d_t player_transform = {
        player_rot->x, -player_rot->y, (map->min.x - player_pos->x) * map->size_inv + MAP_SCALE,
        player_rot->y, player_rot->x, (map->min.y - player_pos->z) * map->size_inv + MAP_SCALE,
    };

    menu_transform_to_fixed(mtx, player_transform);
    data_cache_hit_writeback(mtx, sizeof(transform_2d_fp_t));

    menu_mtx(mtx, true, true);

    material_apply(&map->solid_color->apply);
    rdpq_set_prim_register_raw((color_t){10, 200, 255, 255}, 0 , 0);
    menu_vtx(player_arrow, 0, 3);
    menu_tri(0, 1, 2);
    
    menu_mtx_pop(1);
}

enum map_color_index {
    MAP_COLOR_HIGHLIGHT,
    MAP_COLOR_VISIBLE,
};

static color_t menu_color_table[] = {
    {10, 200, 240, 255},
    {200, 200, 180, 255},
};

#define ICON_SIZE       8
#define FONT_CHAR_WIDTH 6

struct menu_icon_counts {
    uint8_t counts[MAP_ICON_TYPE_COUNT];
    uint8_t kinds_present;
};

typedef struct menu_icon_counts menu_icon_counts_t;

void menu_count_icons(menu_map_layer_room_t* room, menu_icon_counts_t* counts) {
    *counts = (menu_icon_counts_t){};
    
    for (int i = 0; i < room->icon_count; i += 1) {
        menu_map_icon_t* icon = &room->icons[i];

        if (expression_get_bool(icon->hidden)) {
            continue;
        }

        ++counts->counts[icon->icon_type];
    }

    for (int i = 0; i < MAP_ICON_TYPE_COUNT; i += 1) {
        if (counts->counts[i]) {
            ++counts->kinds_present;
        }
    }
}

void menu_render_room_icons(menu_icon_counts_t* counts, int layer_room_index) {
    if (!counts->kinds_present) {
        return;
    }

    int y = -counts->kinds_present * (ICON_SIZE / 2);

    for (int i = 0; i < MAP_ICON_TYPE_COUNT; i += 1) {
        if (!counts->counts[i]) {
            continue;
        }
        
        menu_relative_tex_rect(
            layer_room_index, TILE0, 
            -ICON_SIZE << 1, y << 2, 
            ICON_SIZE << 1, (y + ICON_SIZE) << 2,
            (i * ICON_SIZE) << 5, 0,
            1 << 10, 1 << 10
        );

        y += ICON_SIZE;
    }
}

void menu_render_room_icon_counts(menu_icon_counts_t* counts, int layer_room_index) {
    if (!counts->kinds_present) {
        return;
    }

    int y = -counts->kinds_present * (ICON_SIZE / 2);

    for (int i = 0; i < MAP_ICON_TYPE_COUNT; i += 1) {
        if (!counts->counts[i]) {
            continue;
        }

        char count_text[5];
        sprintf(count_text, "%d", (int)counts->counts[i]);

        int x = ICON_SIZE / 2;

        char* curr_digit = count_text;

        while (*curr_digit) {
            int index = *curr_digit - '0';

            if (index < 0 || index > 9) {
                continue;
            }

            menu_relative_tex_rect(
                layer_room_index, TILE0, 
                x << 2, y << 2, 
                (x + FONT_CHAR_WIDTH) << 2, (y + ICON_SIZE) << 2,
                (index * FONT_CHAR_WIDTH) << 5, 0,
                1 << 10, 1 << 10
            );

            x += FONT_CHAR_WIDTH;
            curr_digit += 1;
        }
        

        y += ICON_SIZE;
    }
}

void menu_setup_layer(menu_map_t* map, menu_map_show_state_t* show_state) {
    menu_map_layer_t* layer = &map->layers[show_state->layer];

    int icon_count = 0;
    
    for (int room_index = 0; room_index < layer->room_count; room_index += 1) {
        icon_count += layer->rooms[room_index].icon_count;
    }

    show_state->icon_vertices = layer->room_count ? malloc(sizeof(menu2d_vtx_t) * layer->room_count) : NULL;
    menu2d_vtx_t* vtx = show_state->icon_vertices;

    rspq_block_begin();

    material_apply(&map->outline_material->apply);
    menu_set_color_table((void*)PhysicalAddr(menu_color_table));

    for (int layer_room_index = 0; layer_room_index < layer->room_count; layer_room_index += 1) {
        menu_map_layer_room_t* room = &layer->rooms[layer_room_index];

        *vtx = (menu2d_vtx_t){
            .pos = room->center,
            .color = {255, 255, 255, 255},
        };
        vtx++;

        int room_index = room->room_index;

        if (!expression_get_bool(current_scene->room_metadata[room_index].has_visited)) {
            continue;
        }

        if (show_state->room_index == room_index) {
            menu_set_color(MENU_COLOR_TARGET_PRIM, MAP_COLOR_HIGHLIGHT);
        } else {
            menu_set_color(MENU_COLOR_TARGET_PRIM, MAP_COLOR_VISIBLE);
        }

        rspq_block_run(layer->rooms[layer_room_index].outline.block);
    }

    if (show_state->icon_vertices) {
        data_cache_hit_writeback_invalidate(show_state->icon_vertices, sizeof(menu2d_vtx_t) * icon_count);
        menu_vtx((void*)PhysicalAddr(show_state->icon_vertices), 0, icon_count);
    }

    menu_icon_counts_t counts[layer->room_count];
    
    for (int layer_room_index = 0; layer_room_index < layer->room_count; layer_room_index += 1) {
        menu_map_layer_room_t* room = &layer->rooms[layer_room_index];

        if (!expression_get_bool(current_scene->room_metadata[room->room_index].has_visited)) {
            counts[layer_room_index] = (menu_icon_counts_t){};
            continue;
        }

        menu_count_icons(room, &counts[layer_room_index]);
    }

    material_apply(&map->map_icon_material->apply);

    for (int layer_room_index = 0; layer_room_index < layer->room_count; layer_room_index += 1) {
        menu_render_room_icons(&counts[layer_room_index], layer_room_index);
    }

    material_apply(&map->number_font_material->apply);
    
    for (int layer_room_index = 0; layer_room_index < layer->room_count; layer_room_index += 1) {
        menu_render_room_icon_counts(&counts[layer_room_index], layer_room_index);
    }

    show_state->block = rspq_block_end();
}

void menu_teardown_show_state(void* data) {
    menu_map_show_state_t* show_state = (menu_map_show_state_t*)data;

    if (show_state->block) {
        rspq_block_free(show_state->block);
    }

    free(show_state->icon_vertices);
}

void menu_map_show(menu_map_t* map, menu_map_show_state_t* show_state, uint16_t room_index) {
    vector3_t* player_pos = player_get_position(&current_scene->player);
    vector2_t* player_rot = player_get_rotation(&current_scene->player);

    show_state->block = NULL;
    show_state->icon_vertices = NULL;
    show_state->offset = (vector2_t){};
    show_state->scale = 1.0f;
    show_state->layer = 0;
    show_state->room_index = 0;

    if (room_index >= map->room_count) {
        return;
    }

    menu_map_room_t* room = &map->rooms[room_index];

    int layer_index = room->min_layer;

    for (int i = 0; i < room->layer_count; i += 1) {
        if (player_pos->y < room->max_layer_y[i]) {
            break;
        }
        
        layer_index += 1;
    }

    if (layer_index >= map->layer_count) {
        return;
    }

    show_state->room_index = room_index;
    show_state->layer = layer_index;

    menu_setup_layer(map, show_state);
}

void menu_map_hide(menu_map_t* map, menu_map_show_state_t* show_state) {
    rdpq_call_deferred(menu_teardown_show_state, show_state);
}

void menu_map_render(menu_map_t* map, menu_map_show_state_t* show_state) {
    menu_common_render_background(20, 20, 200, 200);

    if (!show_state->block) {
        return;
    }
    
    menu_set_viewport(20, 20, 220, 220);
    rdpq_set_scissor(20, 20, 220, 220);
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&transform_test), true, true);

    rspq_block_run(show_state->block);
    
    menu_map_render_player(map);
    
    menu_mtx_pop(1);
    
    rdpq_set_scissor(0, 0, SCREEN_WD, SCREEN_HT);

    // uint16_t* tmp = menu_get_state();

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