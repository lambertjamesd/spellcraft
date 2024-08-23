#include "inventory_menu.h"

#include "menu_common.h"
#include "../resource/material_cache.h"

void inventory_menu_init(struct inventory_menu* menu) {
    menu->cursor_x = 0;
    menu->cursor_y = 0;

    menu->assets.staff_icons[0] = material_cache_load("rom:/materials/objects/icons/default_staff_icon.mat");
    menu->assets.staff_icons[1] = NULL;
    menu->assets.staff_icons[2] = NULL;
    menu->assets.staff_icons[3] = NULL;
}

void inventory_menu_destroy(struct inventory_menu* menu) {
    material_cache_release(menu->assets.staff_icons[0]);
}

void inventory_menu_show(struct inventory_menu* menu) {

}

void inventory_menu_hide(struct inventory_menu* menu) {

}

void inventory_menu_update(struct inventory_menu* menu) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.a && menu->cursor_x < INV_STAFF_COUNT && inventory_has_item(staff_item_types[menu->cursor_x])) {
        inventory_equip_staff(staff_item_types[menu->cursor_x]);
    }
}

#define MENU_X  20
#define MENU_Y  20
#define MENU_W  200
#define MENU_H  200

#define ICON_W  32
#define ICON_H  32

#define COL_COUNT   4

#define COL_SPACING ((MENU_W - COL_COUNT * ICON_W) / (COL_COUNT + 1))

#define ICON_X(col) (COL_SPACING + (col) * (COL_SPACING + ICON_W))

void inventory_menu_render(struct inventory_menu* menu) {
    menu_common_render_background(MENU_X, MENU_Y, MENU_W, MENU_H);

    struct staff_stats* equipped_staff = inventory_equipped_staff();

    if (equipped_staff->item_type != ITEM_TYPE_NONE) {
        int x = MENU_X + ICON_X(equipped_staff->staff_index);
        int y = MENU_Y + COL_SPACING;

        rspq_block_run(current_spell_icon->block);
        rdpq_texture_rectangle_scaled(
            TILE0, 
            x, y,
            x + 32, y + 32,
            0, 0,
            32, 32
        );
    }

    for (int i = 0; i < INV_STAFF_COUNT; i += 1) {
        struct material* material = menu->assets.staff_icons[i];

        if (!material) {
            continue;
        }

        if (!inventory_has_item(staff_item_types[i])) {
            continue;
        }

        rspq_block_run(material->block);

        int icon_x = MENU_X + ICON_X(i);

        rdpq_texture_rectangle_scaled(
            TILE0, 
            icon_x, MENU_Y + COL_SPACING,
            icon_x + ICON_W, MENU_Y + COL_SPACING + ICON_H,
            0, 0,
            material->tex0.sprite->width, material->tex0.sprite->height
        );
    }

    rspq_block_run(spell_cursor_material->block);

    int x = MENU_X + ICON_X(menu->cursor_x);
    int y = MENU_Y + COL_SPACING;

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + 32, y + 32,
        0, 0,
        spell_cursor_material->tex0.sprite->width, spell_cursor_material->tex0.sprite->height
    );
}