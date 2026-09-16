#include "spell_menu.h"

#include "menu_common.h"
#include "../resource/material_cache.h"
#include "../resource/sprite_cache.h"
#include "rsp_menu.h"
#include "../time/time.h"
#include "../util/input.h"
#include "../render/frame_alloc.h"
#include "../math/mathf.h"
#include "../cutscene/globals.h"
#include "../math/constants.h"
#include "../render/defs.h"
#include "../spell/spell.h"
#include "../util/cleanup.h"

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

vector2s16_t spell_directions[4] = {
    {{{1, 0}}},
    {{{-1, 0}}},
    {{{0, 1}}},
    {{{0, -1}}},
};

static inline vector2s16_t spell_menu_spell_start(int direction_index) {
    return (vector2s16_t){{{
        spell_directions[direction_index].x * (START_OFFSET << 2), 
        spell_directions[direction_index].y * (START_OFFSET << 2)
    }}};
}

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

void spell_menu_render_menu_lines(int spell_index, float spell_level) {
    vector2s16_t start = spell_menu_spell_start(spell_index);

    vector2s16_t dir = spell_directions[spell_index];

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

static const char* spell_filename_base = "rom:/images/spell/icons/";

void spell_menu_runepattern_filename(rune_pattern_t pattern, char* filename) {
    switch (pattern.primary_rune) {
        case SPELL_SYMBOL_FIRE:
            sprintf(
                filename, 
                "%sf-%c%c%c0.sprite", 
                spell_filename_base, 
                pattern.windy ? 'a' : '0', 
                pattern.watery ? 'w' : '0', 
                pattern.earthy ? 'e' : '0'
            );
            break;
        case SPELL_SYMBOL_WATER:
            sprintf(
                filename, 
                "%sw-%c%c%c0.sprite", 
                spell_filename_base, 
                pattern.windy ? 'a' : '0', 
                pattern.flaming ? 'f' : '0', 
                pattern.earthy ? 'e' : '0'
            );
            break;
        case SPELL_SYMBOL_EARTH:
            sprintf(
                filename, 
                "%se-%c%c%c0.sprite", 
                spell_filename_base, 
                pattern.watery ? 'w' : '0', 
                pattern.windy ? 'a' : '0', 
                pattern.flaming ? 'f' : '0'
            );
            break;
        case SPELL_SYMBOL_AIR:
            sprintf(
                filename, 
                "%sa-%c%c%c0.sprite", 
                spell_filename_base, 
                pattern.watery ? 'w' : '0', 
                pattern.earthy ? 'e' : '0', 
                pattern.flaming ? 'f' : '0'
            );
            break;
        default:
            assert(false);
            break;
    }
}

rune_pattern_t spell_menu_pattern_from_index(int index) {
    rune_pattern_t result = (rune_pattern_t){};
    result.primary_rune = ((index >> 3) & 0x3) + 1;

    switch (result.primary_rune) {
        case SPELL_SYMBOL_FIRE:
            result.windy = (index >> 2) & 0x1;
            result.watery = (index >> 1) & 0x1;
            result.earthy = (index >> 0) & 0x1;
            break;
        case SPELL_SYMBOL_WATER:
            result.windy = (index >> 2) & 0x1;
            result.flaming = (index >> 1) & 0x1;
            result.earthy = (index >> 0) & 0x1;
            break;
        case SPELL_SYMBOL_EARTH:
            result.watery = (index >> 2) & 0x1;
            result.windy = (index >> 1) & 0x1;
            result.flaming = (index >> 0) & 0x1;
            break;
        case SPELL_SYMBOL_AIR:
            result.watery = (index >> 2) & 0x1;
            result.earthy = (index >> 1) & 0x1;
            result.flaming = (index >> 0) & 0x1;
            break;
        default:
            assert(false);
            break;
    }

    return result;
}

int spell_menu_pattern_to_index(rune_pattern_t pattern) {
    int result = (pattern.primary_rune - 1) << 3;

    switch (pattern.primary_rune) {
        case SPELL_SYMBOL_FIRE:
            result |= (pattern.windy << 2) | (pattern.watery << 1) | (pattern.earthy << 0);
            break;
        case SPELL_SYMBOL_WATER:
            result |= (pattern.windy << 2) | (pattern.flaming << 1) | (pattern.earthy << 0);
            break;
        case SPELL_SYMBOL_EARTH:
            result |= (pattern.watery << 2) | (pattern.windy << 1) | (pattern.flaming << 0);
            break;
        case SPELL_SYMBOL_AIR:
            result |= (pattern.watery << 2) | (pattern.earthy << 1) | (pattern.flaming << 0);
            break;
        default:
            assert(false);
            break;
    }

    return result;
}

vector2s16_t spell_menu_rune_position(int spell_index) {
    rune_pattern_t rune = spell_menu_pattern_from_index(spell_index);

    vector2s16_t result = spell_menu_spell_start(rune.primary_rune - 1);

    switch (rune.primary_rune)
    {
        case SPELL_SYMBOL_FIRE:
            if (rune.earthy) {
                result.x -= SIDE_DISTANCE << 2;
                result.y += SIDE_DISTANCE << 2;
            }
            if (rune.watery) {
                result.x -= SIDE_DISTANCE << 2;
            }
            if (rune.windy) {
                result.x -= SIDE_DISTANCE << 2;
                result.y -= SIDE_DISTANCE << 2;
            }
            break;
        case SPELL_SYMBOL_WATER:
            if (rune.earthy) {
                result.x += SIDE_DISTANCE << 2;
                result.y += SIDE_DISTANCE << 2;
            }
            if (rune.flaming) {
                result.x += SIDE_DISTANCE << 2;
            }
            if (rune.windy) {
                result.x += SIDE_DISTANCE << 2;
                result.y -= SIDE_DISTANCE << 2;
            }
            break;
        case SPELL_SYMBOL_EARTH:
            if (rune.watery) {
                result.x -= SIDE_DISTANCE << 2;
                result.y -= SIDE_DISTANCE << 2;
            }
            if (rune.windy) {
                result.y -= SIDE_DISTANCE << 2;
            }
            if (rune.flaming) {
                result.x += SIDE_DISTANCE << 2;
                result.y -= SIDE_DISTANCE << 2;
            }
            break;
        case SPELL_SYMBOL_AIR:
            if (rune.watery) {
                result.x -= SIDE_DISTANCE << 2;
                result.y += SIDE_DISTANCE << 2;
            }
            if (rune.earthy) {
                result.y += SIDE_DISTANCE << 2;
            }
            if (rune.flaming) {
                result.x += SIDE_DISTANCE << 2;
                result.y += SIDE_DISTANCE << 2;
            }
            break;
        default:
            assert(false);
            break;
    }

    return result;
}

void spell_menu_init(struct spell_menu* spell_menu) {
    spell_menu->offset = gZeroVec2;
    spell_menu->scale = MIN_ZOOM;
    spell_menu->appear_index = -1;

}

void spell_menu_destroy(struct spell_menu* spell_menu) {

}

void spell_menu_show(struct spell_menu* spell_menu) {
    spell_menu->appear_timer = 0.0f;
    spell_menu->appear_index = -1;
    spell_menu->scale = MIN_ZOOM;

    spell_menu->offset = gZeroVec2;

    char icon_filename[64];
    for (int i = 0; i < SPELL_ICON_COUNT; i += 1) {
        spell_menu_runepattern_filename(spell_menu_pattern_from_index(i), icon_filename);
        spell_menu->spell_icons[i] = sprite_cache_load(icon_filename);
    }

    spell_menu->spell_icon_vertices = malloc(sizeof(menu2d_vtx_t) * 4 * SPELL_ICON_COUNT);

    for (int i = 0; i < SPELL_ICON_COUNT; i += 1) {
        spell_menu->spell_icon_vertices[i].pos = spell_menu_rune_position(i);
        spell_menu->spell_icon_vertices[i].color = (color_t){};
        spell_menu->spell_icon_vertices[i].uv = (vector2s16_t){};
    }
    
    spell_menu->solid_color = material_cache_load("rom:/materials/menu/solid_primitive.mat");
    spell_menu->spell_material = material_cache_load("rom:/materials/menu/prim_tex_alpha.mat");

    spell_menu_show_rune_upgrade(spell_menu, SPELL_SYMBOL_FIRE);
}

void spell_menu_hide(struct spell_menu* spell_menu) {
    for (int i = 0; i < SPELL_ICON_COUNT; i += 1) {
        sprite_cache_release(spell_menu->spell_icons[i]);
    }

    cleanup_safe(free, spell_menu->spell_icon_vertices);
    material_cache_release(spell_menu->solid_color);
    material_cache_release(spell_menu->spell_material);
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

    spell_menu->offset.x -= input_handle_deadzone(input.stick_x) * scroll_speed;
    spell_menu->offset.y += input_handle_deadzone(input.stick_y) * scroll_speed;

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

    rdpq_set_scissor(MAP_X, MAP_Y, MAP_X + MAP_SIZE, MAP_Y + MAP_SIZE);

    material_apply(&spell_menu->solid_color->apply);
    
    transform_2d_fp_t* mtx = frame_malloc(frame_pool_curr(), sizeof(transform_2d_fp_t));

    if (spell_menu->appear_index != -1) {
        spell_menu_camera_animation(spell_menu);
    }

    transform_2d_t transform = {
        spell_menu->scale, 0.0f, (MAP_X + MAP_SIZE * 0.5f + spell_menu->offset.x * spell_menu->scale) * FIXED_POINT_SCALE,
        0.0f, spell_menu->scale, (MAP_X + MAP_SIZE * 0.5f + spell_menu->offset.y * spell_menu->scale) * FIXED_POINT_SCALE
    };

    menu_transform_to_fixed(UncachedAddr(mtx), transform);
    
    menu_mtx((transform_2d_fp_t*)PhysicalAddr(mtx), true, true);

    float levels[RUNE_COUNT];

    for (int i = 0; i < RUNE_COUNT; i += 1) {
        levels[i] = global_load_location(rune_count_variables[i]);

        if (spell_menu->appear_index == i) {
            if (spell_menu->appear_timer < APPEAR_LINE_APPEAR_START) {
                levels[i] -= 1.0f;
            } else if (spell_menu->appear_timer < APPEAR_LINE_APPEAR_END) {
                levels[i] -= 1.0f - (spell_menu->appear_timer - APPEAR_LINE_APPEAR_START) * (1.0f / (APPEAR_LINE_APPEAR_END - APPEAR_LINE_APPEAR_START));
            }
        }

        spell_menu_render_menu_lines(i, levels[i]);
    }

    menu_vtx(spell_menu->spell_icon_vertices, 0, SPELL_ICON_COUNT);

    material_apply(&spell_menu->spell_material->apply);

    float icon_scale = mathfLerp(0.5f, 1.0f, mathfInvLerp(MIN_ZOOM, MAX_ZOOM, spell_menu->scale));

    int shift = (int)((1 << 10) / icon_scale);
    int half_size = (int)(16 * FIXED_POINT_SCALE * icon_scale);

    for (int i = 0; i < SPELL_ICON_COUNT; i += 1) {
        rune_pattern_t rune = spell_menu_pattern_from_index(i);

        float level = levels[rune.primary_rune-1];

        int symbol_level = rune_pattern_symbol_count(rune) - 1;

        if (symbol_level >= floorf(level)) {
            continue;
        }

        vector2s16_t local_pos = spell_menu->spell_icon_vertices[i].pos;
        vector2s16_t anchor = (vector2s16_t){{{
            (short)(local_pos.x * spell_menu->scale + transform[2]),
            (short)(local_pos.y * spell_menu->scale + transform[5]),
        }}};

        rdpq_sync_pipe();
        rdpq_sync_tile();
        surface_t surf = sprite_get_pixels(spell_menu->spell_icons[i]);
        rdpq_tex_upload(TILE0, &surf, NULL);
        rdpq_set_prim_register_raw((color_t){255, i * 8, 0, 255}, 0, 0);
        __rdpq_texture_rectangle_raw_fx(
            TILE0,
            anchor.x - half_size, anchor.y - half_size,
            anchor.x + half_size, anchor.y + half_size,
            0, 0, shift, shift
        );
    }
    
    menu_mtx_pop(1);
    
    rdpq_set_scissor(0, 0, SCREEN_WD, SCREEN_HT);
}