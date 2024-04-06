#include "spell_menu.h"

#include "menu_common.h"

void spell_menu_init(struct spell_menu* spell_menu) {
    spell_menu->cursor_x = 0;
    spell_menu->cursor_y = 0;
}

void spell_menu_destroy(struct spell_menu* spell_menu) {

}

void spell_menu_show(struct spell_menu* spell_menu) {
    spell_menu->cursor_x = 0;
    spell_menu->cursor_y = 0;
}

void spell_menu_hide(struct spell_menu* spell_menu) {

}

void spell_menu_assign_slot(struct spell_menu* menu, int slot) {
    struct spell* selected_spell;
    
    if (menu->cursor_y == 3) {
        selected_spell = inventory_get_custom_spell(menu->cursor_x);
    } else {
        selected_spell = inventory_get_built_in_spell(menu->cursor_x, menu->cursor_y);
    }

    if (!selected_spell) {
        return;
    }

    int existing_slot = -1;

    for (int i = 0; i < MAX_SPELL_SLOTS; i += 1) {
        if (inventory_get_equipped_spell(i) == selected_spell) {
            existing_slot = i;
            break;
        }
    }

    if (existing_slot != -1) {
        // if already equipped, then swap
        inventory_set_equipped_spell(existing_slot, inventory_get_equipped_spell(slot));
    }
    inventory_set_equipped_spell(slot, selected_spell);
}

struct spell* spell_menu_update(struct spell_menu* spell_menu) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.c_up) {
        spell_menu_assign_slot(spell_menu, 0);
    }

    if (pressed.c_down) {
        spell_menu_assign_slot(spell_menu, 1);
    }

    if (pressed.c_left) {
        spell_menu_assign_slot(spell_menu, 2);
    }

    if (pressed.c_right) {
        spell_menu_assign_slot(spell_menu, 3);
    }

    if (pressed.a && spell_menu->cursor_y == 3) {
        return inventory_get_custom_spell(spell_menu->cursor_x);
    }

    int direction = joypad_get_axis_pressed(0, JOYPAD_AXIS_STICK_X);

    if (direction > 0 && spell_menu->cursor_x + 1 < INVENTORY_SPELL_COLUMNS) {
        spell_menu->cursor_x += 1;
    }

    if (direction < 0 && spell_menu->cursor_x > 0) {
        spell_menu->cursor_x -= 1;
    }

    direction = joypad_get_axis_pressed(0, JOYPAD_AXIS_STICK_Y);

    if (direction < 0 && spell_menu->cursor_y + 1 <= INVENTORY_SPELL_ROWS) {
        spell_menu->cursor_y += 1;
    }

    if (direction > 0 && spell_menu->cursor_y > 0) {
        spell_menu->cursor_y -= 1;
    }

    return NULL;
}

#define SPELL_SYMBOL_X(x) ((x) * 32 + 28)
#define SPELL_SYMBOL_Y(y) ((y) * 32 + 28 + 32)

void spell_menu_render(struct spell_menu* spell_menu) {
    menu_common_render_background(20, 20, 200, 200);

    for (int row = 0; row < INVENTORY_SPELL_ROWS; row += 1) {
        for (int col = 0; col < INVENTORY_SPELL_COLUMNS; col += 1) {
            struct spell* spell = inventory_get_built_in_spell(col, row);

            if (spell) {
                rspq_block_run(menu_spell_icons[spell->symbol_index]->block);

                int x = SPELL_SYMBOL_X(col);
                int y = SPELL_SYMBOL_Y(row);

                rdpq_texture_rectangle(
                    TILE0,
                    x, y,
                    x + 24, y + 24,
                    0, 0
                );
            }
        }
    }

    rspq_block_run(spell_cursor_material->block);

    int x = SPELL_SYMBOL_X(spell_menu->cursor_x);
    int y = SPELL_SYMBOL_Y(spell_menu->cursor_y == 3 ? spell_menu->cursor_y + 1 : spell_menu->cursor_y);

    rdpq_texture_rectangle(
        TILE0,
        x, y,
        x + 24, y + 24,
        0, 0
    );

    for (int i = 0; i < 6; i += 1) {
        rspq_block_run(menu_spell_icons[i + SPELL_ICON_CUSTOM_0]->block);

        int x = SPELL_SYMBOL_X(i);
        int y = SPELL_SYMBOL_Y(4);

        rdpq_texture_rectangle(
            TILE0,
            x, y,
            x + 24, y + 24,
            0, 0
        );
    }
}