#include "game_mode.h"

#include <stdint.h>

enum game_mode current_game_mode = GAME_MODE_3D;
extern uint8_t blur_frame_counter;

void game_mode_enter_menu() {
    if (current_game_mode == GAME_MODE_3D) {
        current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
    }
}

void game_mode_exit_menu() {
    current_game_mode = GAME_MODE_3D;
}

void game_mode_blur_background() {
    blur_frame_counter = 0;
}