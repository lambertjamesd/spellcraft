#include "pause_menu.h"

#include <libdragon.h>
#include "../time/time.h"
#include "../time/game_mode.h"
#include "menu_rendering.h"

void pause_menu_render(void *data) {
    struct pause_menu* pause_menu = (struct pause_menu*)data;

    switch (pause_menu->active_menu) {
        case ACTIVE_MENU_SPELL_BUILDING:
            spell_building_render_menu(&pause_menu->spell_building_menu);
            break;
        case ACTIVE_MENU_SPELLS:
            spell_menu_render(&pause_menu->spell_menu);
            break;
        case ACTIVE_MENU_INVENTORY:
            inventory_menu_render(&pause_menu->inventory_menu);
            break;
        default:
            break;
    }
}

void pause_menu_transition(struct pause_menu* pause_menu, enum active_menu target, void* data) {
    if (pause_menu->active_menu == target) {
        return;
    }

    switch (pause_menu->active_menu) {
        case ACTIVE_MENU_NONE:
            current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
            update_pause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_CUTSCENE);
            break;
        case ACTIVE_MENU_SPELL_BUILDING:
            spell_building_menu_hide(&pause_menu->spell_building_menu);
            break;
        case ACTIVE_MENU_SPELLS:
            spell_menu_hide(&pause_menu->spell_menu);
            break;
        case ACTIVE_MENU_INVENTORY:
            inventory_menu_hide(&pause_menu->inventory_menu);
            break;
    }

    pause_menu->active_menu = target;

    switch (target) {
        case ACTIVE_MENU_NONE:
            current_game_mode = GAME_MODE_3D;
            update_unpause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_CUTSCENE);
            break;
        case ACTIVE_MENU_SPELL_BUILDING:
            spell_building_menu_show(&pause_menu->spell_building_menu, data);
            break;
        case ACTIVE_MENU_SPELLS:
            spell_menu_show(&pause_menu->spell_menu);
            break;
        case ACTIVE_MENU_INVENTORY:
            inventory_menu_show(&pause_menu->inventory_menu);
            break;
    }
}

static enum active_menu next_menu[] = {
    [ACTIVE_MENU_NONE] = ACTIVE_MENU_NONE,
    [ACTIVE_MENU_SPELLS] = ACTIVE_MENU_INVENTORY,
    [ACTIVE_MENU_SPELL_BUILDING] = ACTIVE_MENU_SPELLS,
    [ACTIVE_MENU_INVENTORY] = ACTIVE_MENU_SPELLS,
};

static enum active_menu prev_menu[] = {
    [ACTIVE_MENU_NONE] = ACTIVE_MENU_NONE,
    [ACTIVE_MENU_SPELLS] = ACTIVE_MENU_INVENTORY,
    [ACTIVE_MENU_SPELL_BUILDING] = ACTIVE_MENU_SPELLS,
    [ACTIVE_MENU_INVENTORY] = ACTIVE_MENU_SPELLS,
};

void pause_menu_update(struct pause_menu* pause_menu) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.start) {
        if (pause_menu->active_menu == ACTIVE_MENU_NONE) {
            pause_menu_transition(pause_menu, ACTIVE_MENU_SPELLS, NULL);
        } else {
            pause_menu_transition(pause_menu, ACTIVE_MENU_NONE, NULL);
        }
    } else if (pressed.r) {
        pause_menu_transition(pause_menu, next_menu[pause_menu->active_menu], NULL);
    } else if (pressed.z) {
        pause_menu_transition(pause_menu, prev_menu[pause_menu->active_menu], NULL);
    } else if (pause_menu->active_menu != ACTIVE_MENU_NONE) {
        switch (pause_menu->active_menu) {
            case ACTIVE_MENU_SPELL_BUILDING:
                spell_building_menu_update(&pause_menu->spell_building_menu);
                break;
            case ACTIVE_MENU_SPELLS: {
                struct spell* edit_spell = spell_menu_update(&pause_menu->spell_menu);

                if (edit_spell) {
                    pause_menu_transition(pause_menu, ACTIVE_MENU_SPELL_BUILDING, edit_spell);
                }

                break;
            }
            case ACTIVE_MENU_INVENTORY:
                inventory_menu_update(&pause_menu->inventory_menu);
                break;
            default:
                break;
        }
    }
}

void pause_menu_init(struct pause_menu* pause_menu) {
    update_add(pause_menu, (update_callback)pause_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_PAUSE_MENU);
    spell_building_menu_init(&pause_menu->spell_building_menu);
    spell_menu_init(&pause_menu->spell_menu);
    menu_add_callback(pause_menu_render, pause_menu, 0);
    inventory_menu_init(&pause_menu->inventory_menu);
    pause_menu->active_menu = ACTIVE_MENU_NONE;
}

void pause_menu_destroy(struct pause_menu* pause_menu) {
    update_remove(pause_menu);
    spell_building_menu_destroy(&pause_menu->spell_building_menu);
    spell_menu_destroy(&pause_menu->spell_menu);
    menu_remove_callback(pause_menu);
}