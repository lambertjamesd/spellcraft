#include "spell_menu.h"

#include "menu_common.h"
#include "../resource/material_cache.h"
#include "rsp_menu.h"
#include "../time/time.h"
#include "../util/input.h"
#include "../render/frame_alloc.h"

#define SIDE_DISTANCE   (40 << 2)
#define LERP_MARGIN     0x2000

#define START_OFFSET    ((SIDE_DISTANCE * 3) + (SIDE_DISTANCE >> 1))

#define SCROLL_SCALE    (256.0f / 80.0f)

void spell_menu_render_coord(vector2s16_t* start, vector2s16_t* edge_directions, int code, vector2s16_t* result) {
    *result = *start;
    for (int i = 0; i < 3; i += 1) {
        if ((1 << i) & code) {
            vector2s16Add(result, &edge_directions[i], result);
        }
    }
}

void spell_menu_render_connection(vector2s16_t* start, vector2s16_t* edge_directions, int a_code, int b_code, float lerp) {
    vector2s16_t a;
    vector2s16_t b;

    spell_menu_render_coord(start, edge_directions, a_code, &a);
    spell_menu_render_coord(start, edge_directions, b_code, &b);


    vector2s16_t point;
    vector2s16Lerp(&a, &b, LERP_MARGIN, &point);

    menu_move_to(&(menu2d_line_vtx_t){
        .pos = point,
        .width = 16,
    });
    
    vector2s16Lerp(&a, &b, (uint16_t)((0xFFFF - 2 * LERP_MARGIN) * lerp) + LERP_MARGIN, &point);
    
    menu_line_to(&(menu2d_line_vtx_t){
        .pos = point,
        .width = 16,
    });
}

vector2s16_t spell_directions[4] = {
    {{{1, 0}}},
    {{{-1, 0}}},
    {{{0, 1}}},
    {{{0, -1}}},
};

void spell_menu_render_menu_lines(int spell_index, float spell_level) {
    vector2s16_t dir = spell_directions[spell_index];

    vector2s16_t start = {{{dir.x * START_OFFSET, dir.y * START_OFFSET}}};

    dir.x *= SIDE_DISTANCE;
    dir.y *= SIDE_DISTANCE;

    vector2s16_t cross_dir;
    vector2s16Rotate90(&dir, &cross_dir);

    vector2s16_t edge_directions[3] = {
        {{{-dir.x + cross_dir.x, -dir.y + cross_dir.y}}},
        {{{-dir.x, -dir.y}}},
        {{{-dir.x - cross_dir.x, -dir.y - cross_dir.y}}},
    };

    if (spell_level < 1.0f) {
        return;
    }

    float lerp = spell_level - 1.0f;

    if (lerp > 1.0f) {
        lerp = 1.0f;
    }

    spell_menu_render_connection(&start, edge_directions, 0, 1, lerp);
    spell_menu_render_connection(&start, edge_directions, 0, 2, lerp);
    spell_menu_render_connection(&start, edge_directions, 0, 4, lerp);

    if (spell_level < 2.0f) {
        return;
    }

    lerp = spell_level - 2.0f;

    if (lerp > 1.0f) {
        lerp = 1.0f;
    }

    spell_menu_render_connection(&start, edge_directions, 1, 3, lerp);
    spell_menu_render_connection(&start, edge_directions, 1, 5, lerp);
    
    spell_menu_render_connection(&start, edge_directions, 2, 3, lerp);
    spell_menu_render_connection(&start, edge_directions, 2, 6, lerp);
    
    spell_menu_render_connection(&start, edge_directions, 4, 5, lerp);
    spell_menu_render_connection(&start, edge_directions, 4, 6, lerp);
    
    if (spell_level < 3.0f) {
        return;
    }

    lerp = spell_level - 3.0;

    if (lerp > 1.0f) {
        lerp = 1.0f;
    }

    spell_menu_render_connection(&start, edge_directions, 3, 7, lerp);
    spell_menu_render_connection(&start, edge_directions, 5, 7, lerp);
    spell_menu_render_connection(&start, edge_directions, 6, 7, lerp);
}

void spell_menu_init(struct spell_menu* spell_menu) {
    spell_menu->cursor_x = 0;
    spell_menu->cursor_y = 0;

    spell_menu->solid_color = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void spell_menu_destroy(struct spell_menu* spell_menu) {
    material_cache_release(spell_menu->solid_color);
}

void spell_menu_show(struct spell_menu* spell_menu) {
    spell_menu->cursor_x = 0;
    spell_menu->cursor_y = 0;
    spell_menu->appear_timer = 0.0f;

    spell_menu->offset = gZeroVec2;
}

void spell_menu_hide(struct spell_menu* spell_menu) {

}

void spell_menu_update(struct spell_menu* spell_menu) {
    joypad_inputs_t input = joypad_get_inputs(0);

    float scroll_speed = fixed_time_step * SCROLL_SCALE;

    spell_menu->offset.x -= input_handle_deadzone(input.stick_x) * scroll_speed;
    spell_menu->offset.y += input_handle_deadzone(input.stick_y) * scroll_speed;
}

void spell_menu_render(struct spell_menu* spell_menu) {
    menu_common_render_background(20, 20, 200, 200);

    material_apply(&spell_menu->solid_color->apply);
    
    transform_2d_fp_t* mtx = frame_malloc(frame_pool_curr(), sizeof(transform_2d_fp_t));

    menu_transform_to_fixed(UncachedAddr(mtx), (transform_2d_t){
        1.0f, 0.0f, spell_menu->offset.x,
        0.0f, 1.0f, spell_menu->offset.y
    });
    
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(mtx), true, true);

    spell_menu_render_menu_lines(0, 4.0f);
    spell_menu_render_menu_lines(1, 4.0f);
    spell_menu_render_menu_lines(2, 4.0f);
    spell_menu_render_menu_lines(3, 4.0f);
    
    menu_mtx_pop(1);

    spell_menu->appear_timer += fixed_time_step;
}