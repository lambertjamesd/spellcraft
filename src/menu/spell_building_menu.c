#include "spell_building_menu.h"

#include <libdragon.h>

#include "menu_rendering.h"
#include "menu_common.h"
#include "../resource/material_cache.h"
#include "../time/time.h"

static struct material* spell_symbol_material;

#define SPELL_SYMBOLS_PER_ROW   6

#define SPELL_SYMBOL_LOCATION_X(index)  ((((index) - SPELL_SYMBOL_FIRE) % SPELL_SYMBOLS_PER_ROW) * 32 + 40)
#define SPELL_SYMBOL_LOCATION_Y(index)  ((((index) - SPELL_SYMBOL_FIRE) / SPELL_SYMBOLS_PER_ROW) * 32 + 142)

void spell_building_render_menu(struct spell_building_menu* menu) {
    menu_common_render_background(32, 32, 256, 96);

    menu_common_render_background(32, 134, 256, 74);

    rspq_block_run(spell_symbol_material->block);

    for (int row = 0; row < SPELL_MAX_ROWS; ++row) {
        int y = row * 24 + 32;
        for (int col = 0; col < SPELL_MAX_COLS; ++col) {
            struct spell_symbol symbol = menu->symbol_grid[row][col];

            if (symbol.type == ITEM_TYPE_NONE) {
                continue;
            }

            int x = col * 24 + 32;

            int source_x = (symbol.type - 1) * 24;

            rdpq_texture_rectangle_scaled(
                TILE0,
                x, y,
                x + 24, y + 24,
                source_x, 0,
                source_x + 24, 24
            );
        }
    }

    for (int spell_type = SPELL_SYMBOL_FIRE; spell_type < SPELL_SYBMOL_COUNT; spell_type += 1) {
        if (!inventory_has_item(spell_type)) {
            continue;
        }

        int source_x = (spell_type - 1) * 24;
        int x = SPELL_SYMBOL_LOCATION_X(spell_type);
        int y = SPELL_SYMBOL_LOCATION_Y(spell_type);

        rdpq_texture_rectangle_scaled(
            TILE0,
            x, y,
            x + 24, y + 24,
            source_x, 0,
            source_x + 24, 24
        );
    }

    rspq_block_run(spell_cursor_material->block);

    int x = 32 + 8 + menu->symbol_cursor_x * 32;
    int y = 134 + 8 + menu->symbol_cursor_y * 32;

    rdpq_texture_rectangle(
        TILE0,
        x, y,
        x + 24, y + 24,
        0, 0
    );

    x = 32 + menu->spell_cursor_x * 24;
    y = 32 + menu->spell_cursor_y * 24;

    rdpq_texture_rectangle(
        TILE0,
        x, y,
        x + 4, y + 24,
        0, 0
    );
}

void spell_building_menu_update(struct spell_building_menu* menu) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.c_right && menu->spell_cursor_x < SPELL_MAX_COLS) {
        menu->spell_cursor_x += 1;
    }

    if (pressed.c_left && menu->spell_cursor_x > 0) {
        menu->spell_cursor_x -= 1;
    }

    if (pressed.c_down && menu->spell_cursor_y + 1 < SPELL_MAX_ROWS) {
        menu->spell_cursor_y += 1;
    }
    
    if (pressed.c_up && menu->spell_cursor_y > 0) {
        menu->spell_cursor_y -= 1;
    }

    int direction = joypad_get_axis_pressed(0, JOYPAD_AXIS_STICK_X);

    if (direction > 0 && menu->symbol_cursor_x + 1 < SPELL_SYMBOLS_PER_ROW) {
        menu->symbol_cursor_x += 1;
    }

    if (direction < 0 && menu->symbol_cursor_x > 0) {
        menu->symbol_cursor_x -= 1;
    }

    direction = joypad_get_axis_pressed(0, JOYPAD_AXIS_STICK_Y);

    if (direction > 0 && menu->symbol_cursor_y + 1 < 2) {
        menu->symbol_cursor_y += 1;
    }

    if (direction < 0 && menu->symbol_cursor_y > 0) {
        menu->symbol_cursor_y -= 1;
    }

    if (pressed.a) {
        int spell_symbol = menu->symbol_cursor_x + menu->symbol_cursor_y * SPELL_SYMBOLS_PER_ROW + SPELL_SYMBOL_FIRE;

        if (spell_symbol >= SPELL_SYMBOL_FIRE && spell_symbol <= SPELL_SYBMOL_COUNT && inventory_has_item(spell_symbol)) {
            // TODO shift connected symbols below
            for (int i = SPELL_MAX_COLS - 1; i > menu->spell_cursor_x; i -= 1) {
                menu->symbol_grid[menu->spell_cursor_y][i] = menu->symbol_grid[menu->spell_cursor_y][i - 1];
            }

            menu->symbol_grid[menu->spell_cursor_y][menu->spell_cursor_x].type = spell_symbol;

            menu->spell_cursor_x += 1;
        }
    }

    if (pressed.b && menu->spell_cursor_x > 0) {
        for (int i = menu->spell_cursor_x; i <= SPELL_MAX_COLS; i += 1) {
            menu->symbol_grid[menu->spell_cursor_y][i - 1].type = i == SPELL_MAX_COLS ? ITEM_TYPE_NONE : menu->symbol_grid[menu->spell_cursor_y][i].type;
        }

        menu->spell_cursor_x -= 1;
    }
}

void spell_building_menu_init(struct spell_building_menu* menu) {
    spell_symbol_material = material_cache_load("rom:/materials/spell/symbols.mat");
}

void spell_building_menu_destroy(struct spell_building_menu* menu) {
    material_cache_release(spell_symbol_material);
}

void spell_building_menu_show(struct spell_building_menu* menu, struct spell* spell) {
    for (int row = 0; row < SPELL_MAX_ROWS; ++row) {
        for (int col = 0; col < SPELL_MAX_COLS; ++col) {
            menu->symbol_grid[row][col] = spell_get_symbol(spell, col, row);
        }
    }

    menu->current_spell = spell;
}

void spell_building_menu_hide(struct spell_building_menu* menu) {
    for (int row = 0; row < SPELL_MAX_ROWS; ++row) {
        for (int col = 0; col < SPELL_MAX_COLS; ++col) {
            spell_set_symbol(menu->current_spell, col, row, menu->symbol_grid[row][col]);
        }
    }
}