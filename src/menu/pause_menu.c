#include "pause_menu.h"

#include <libdragon.h>
#include "../time/time.h"
#include "../time/game_mode.h"

void pause_menu_update(struct pause_menu* pause_menu) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.start) {
        if (pause_menu->is_open) {
            pause_menu->is_open = 0;

            current_game_mode = GAME_MODE_3D;
            update_unpause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG);
            spell_buliding_menu_hide(&pause_menu->spell_menu);
        } else {
            pause_menu->is_open = 1;

            current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
            update_pause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG);
            spell_building_menu_show(&pause_menu->spell_menu, &pause_menu->inventory->custom_spells[0]);
        }
    }
}

void pause_menu_init(struct pause_menu* pause_menu, struct inventory* inventory) {
    update_add(pause_menu, (update_callback)pause_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_PAUSE_MENU);
    spell_building_menu_init(&pause_menu->spell_menu);
    pause_menu->is_open = 0;
    pause_menu->inventory = inventory;
}

void pause_menu_destroy(struct pause_menu* pause_menu) {
    update_remove(pause_menu);
    spell_building_menu_destroy(&pause_menu->spell_menu);
}