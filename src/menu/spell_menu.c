#include "spell_menu.h"

#include "menu_common.h"
#include "../resource/material_cache.h"
#include "rsp_menu.h"
#include "../time/time.h"
#include "../util/input.h"
#include "../render/frame_alloc.h"
#include "../math/mathf.h"
#include "../cutscene/globals.h"
#include "../math/constants.h"

#define FIXED_POINT_SCALE   4.0f
#define MAP_X           20
#define MAP_Y           20
#define MAP_SIZE        200

#define SIDE_DISTANCE   40
#define LERP_MARGIN     0x2000

#define ZOOM_SPEED      2.5f
#define MAX_ZOOM        2.0f
#define MIN_ZOOM        0.6f

#define LN_MAX_MIN      1.203972804325f

#define RUNE_COUNT      4

#define START_OFFSET    ((SIDE_DISTANCE * 3) + (SIDE_DISTANCE >> 1))

#define SCROLL_SCALE    (128.0f / 80.0f)

#define APPEAR_LINE_APPEAR_START        0.5f
#define APPEAR_LINE_APPEAR_END          1.5f
#define APPEAR_ZOOM_START               2.5f
#define APPEAR_ZOOM_END                 3.0f
#define APPEAR_ANIMATION_DURATION       3.0f

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

    vector2s16_t start = {{{dir.x * (START_OFFSET << 2), dir.y * (START_OFFSET << 2)}}};

    dir.x *= SIDE_DISTANCE << 2;
    dir.y *= SIDE_DISTANCE << 2;

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
    spell_menu->offset = gZeroVec2;
    spell_menu->scale = MIN_ZOOM;
    spell_menu->appear_index = -1;

    spell_menu->solid_color = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void spell_menu_destroy(struct spell_menu* spell_menu) {
    material_cache_release(spell_menu->solid_color);
}

void spell_menu_show(struct spell_menu* spell_menu) {
    spell_menu->appear_timer = 0.0f;
    spell_menu->appear_index = -1;
    spell_menu->scale = MIN_ZOOM;

    spell_menu->offset = gZeroVec2;

    spell_menu_show_rune_upgrade(spell_menu, SPELL_SYMBOL_FIRE);
}

void spell_menu_hide(struct spell_menu* spell_menu) {

}
void spell_menu_show_rune_upgrade(struct spell_menu* spell_menu, enum inventory_item_type item_type) {
    assert(item_type >= SPELL_SYMBOL_FIRE && item_type <= SPELL_SYMBOL_AIR);
    spell_menu->appear_index = item_type - SPELL_SYMBOL_FIRE;
    spell_menu->appear_timer = 0.0f;
}

static global_location_t rune_count_variables[] = {
    VAR_LOC_fire_rune_level,
    VAR_LOC_ice_rune_level,
    VAR_LOC_earth_rune_level,
    VAR_LOC_air_rune_level,
};

void spell_menu_camera_animation(struct spell_menu* spell_menu) {
    int current_level = global_load_location(rune_count_variables[spell_menu->appear_index]);

    float distance = START_OFFSET - SIDE_DISTANCE * (current_level - 1.5f);

    vector2_t center_point = {
        .x = spell_directions[spell_menu->appear_index].x * -distance,
        .y = spell_directions[spell_menu->appear_index].y * -distance,
    };

    if (spell_menu->appear_timer < APPEAR_ZOOM_START) {
        spell_menu->offset = center_point;
        spell_menu->scale = MAX_ZOOM;
    } else if (spell_menu->appear_timer < APPEAR_ZOOM_END) {
        float lerp = (spell_menu->appear_timer - APPEAR_ZOOM_START) * (1.0f / (APPEAR_ZOOM_END - APPEAR_ZOOM_START));

        vector2_t lerp_value;
        vector2Scale(&center_point, 1.0f - lerp, &spell_menu->offset);
        spell_menu->scale = expf((1.0f - lerp) * LN_MAX_MIN) * MIN_ZOOM;
    } else {
        spell_menu->offset = gZeroVec2;
        spell_menu->scale = MIN_ZOOM;
    }
}

void spell_menu_update(struct spell_menu* spell_menu) {
    if (spell_menu->appear_index != -1) {
        spell_menu->appear_timer += fixed_time_step;

        if (spell_menu->appear_timer > APPEAR_ANIMATION_DURATION) {
            spell_menu->appear_timer = APPEAR_ANIMATION_DURATION;
            spell_menu_camera_animation(spell_menu);

            spell_menu->appear_index = -1;
            spell_menu->appear_timer = 0.0f;
        }
        return;
    }

    joypad_inputs_t input = joypad_get_inputs(0);

    float scroll_speed = fixed_time_step * SCROLL_SCALE / spell_menu->scale;

    spell_menu->offset.x += input_handle_deadzone(input.stick_x) * scroll_speed;
    spell_menu->offset.y -= input_handle_deadzone(input.stick_y) * scroll_speed;

    spell_menu->offset.x = clampf(spell_menu->offset.x, -START_OFFSET, START_OFFSET);
    spell_menu->offset.y = clampf(spell_menu->offset.y, -START_OFFSET, START_OFFSET);
    
    if (input.btn.c_right) {
        spell_menu->scale *= powf(ZOOM_SPEED, fixed_time_step);
        if (spell_menu->scale > MAX_ZOOM) {
            spell_menu->scale = MAX_ZOOM;
        }
    } else if (input.btn.c_left) {
        spell_menu->scale *= powf(ZOOM_SPEED, -fixed_time_step);
        if (spell_menu->scale < MIN_ZOOM) {
            spell_menu->scale = MIN_ZOOM;
        }
    }
}

void spell_menu_render(struct spell_menu* spell_menu) {
    menu_common_render_background(20, 20, 200, 200);

    material_apply(&spell_menu->solid_color->apply);
    
    transform_2d_fp_t* mtx = frame_malloc(frame_pool_curr(), sizeof(transform_2d_fp_t));

    if (spell_menu->appear_index != -1) {
        spell_menu_camera_animation(spell_menu);
    }

    menu_transform_to_fixed(UncachedAddr(mtx), (transform_2d_t){
        spell_menu->scale, 0.0f, (MAP_X + MAP_SIZE * 0.5f + spell_menu->offset.x * spell_menu->scale) * FIXED_POINT_SCALE,
        0.0f, spell_menu->scale, (MAP_X + MAP_SIZE * 0.5f + spell_menu->offset.y * spell_menu->scale) * FIXED_POINT_SCALE
    });
    
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(mtx), true, true);

    for (int i = 0; i < RUNE_COUNT; i += 1) {
        float level = global_load_location(rune_count_variables[i]);

        if (spell_menu->appear_index == i) {
            if (spell_menu->appear_timer < APPEAR_LINE_APPEAR_START) {
                level -= 1.0f;
            } else if (spell_menu->appear_timer < APPEAR_LINE_APPEAR_END) {
                level -= 1.0f - (spell_menu->appear_timer - APPEAR_LINE_APPEAR_START) * (1.0f / (APPEAR_LINE_APPEAR_END - APPEAR_LINE_APPEAR_START));
            }
        }

        spell_menu_render_menu_lines(
            i, 
            level
        );
    }
    
    menu_mtx_pop(1);
}