#include "spell_menu.h"

#include "menu_common.h"

void spell_menu_init(struct spell_menu* spell_menu, struct inventory* inventory) {
    spell_menu->inventory = inventory;
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
    struct spell* selected_spell = menu->inventory->built_in_spells[
        menu->cursor_y * INVENTORY_SPELL_COLUMNS + menu->cursor_x
    ];

    if (!selected_spell) {
        return;
    }

    int existing_slot = -1;

    for (int i = 0; i < MAX_SPELL_SLOTS; i += 1) {
        if (menu->inventory->spell_slots[i] == selected_spell) {
            existing_slot = i;
            break;
        }
    }

    if (existing_slot != -1) {
        menu->inventory->spell_slots[existing_slot] = menu->inventory->spell_slots[slot];
    }
    menu->inventory->spell_slots[slot] = selected_spell;
}

void spell_menu_update(struct spell_menu* spell_menu) {
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

    int direction = joypad_get_axis_pressed(0, JOYPAD_AXIS_STICK_X);

    if (direction > 0 && spell_menu->cursor_x + 1 < INVENTORY_SPELL_COLUMNS) {
        spell_menu->cursor_x += 1;
    }

    if (direction < 0 && spell_menu->cursor_x > 0) {
        spell_menu->cursor_x -= 1;
    }
}

void spell_menu_render(struct spell_menu* spell_menu) {
    menu_common_render_background(20, 20, 200, 200);

    struct spell** built_in_spell = spell_menu->inventory->built_in_spells;

    for (int row = 0; row < INVENTORY_SPELL_ROWS; row += 1) {
        for (int col = 0; col < INVENTORY_SPELL_COLUMNS; col += 1) {
            struct spell* spell = *built_in_spell;

            if (spell) {
                rspq_block_run(menu_spell_icons[spell->symbol_index]->block);

                int x = col * 32 + 28;
                int y = row * 32 + 28 + 32;

                rdpq_texture_rectangle(
                    TILE0,
                    x, y,
                    x + 24, y + 24,
                    0, 0
                );
            }

            built_in_spell += 1;
        }
    }

    rspq_block_run(spell_cursor_material->block);

    int x = spell_menu->cursor_x * 32 + 28;
    int y = spell_menu->cursor_y * 32 + 28 + 32;

    rdpq_texture_rectangle(
        TILE0,
        x, y,
        x + 24, y + 24,
        0, 0
    );
}