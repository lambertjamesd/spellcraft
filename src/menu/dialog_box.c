#include "dialog_box.h"

#include "../resource/font_cache.h"
#include "menu_common.h"
#include "menu_rendering.h"
#include "../time/time.h"
#include "../util/text.h"

#define CHARACTERS_PER_SECOND   15.0f

struct dialog_box g_dialog_box;

void dialog_box_init(struct dialog_box* dialog_box) {
    dialog_box->current_message = NULL;
    dialog_box->font = font_cache_load("rom:/fonts/Amarante-Regular.font64");
}

void dialog_box_update(void* data) {
    struct dialog_box* dialog_box = (struct dialog_box*)data;

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (!dialog_box->paused) {
        dialog_box->requested_characters += fixed_time_step * (input.btn.a ? CHARACTERS_PER_SECOND * 2.0f : CHARACTERS_PER_SECOND);

        while (dialog_box->requested_characters >= 1.0f) {
            uint32_t character_point = utf8_decode(&dialog_box->current_message_end);

            if (character_point == 0 || (character_point == '\n' && *dialog_box->current_message_end == '\n')) {
                dialog_box->paused = 1;

                if (character_point == 0) {
                    dialog_box->end_of_message = 1;
                }
            }

            dialog_box->requested_characters -= 1.0f;
        }
    }

    if (dialog_box->paused && pressed.a) {
        if (dialog_box->end_of_message) {
            if (dialog_box->end_callback) {
                dialog_box->end_callback(dialog_box->end_callback_data);
            }
            dialog_box_hide(dialog_box);
        } else {
            dialog_box->current_message_start = dialog_box->current_message_end + 1;
            dialog_box->current_message_end = dialog_box->current_message_start;
            dialog_box->paused = 0;
        }
    }
}

void dialog_box_render(void* data) {
    struct dialog_box* dialog_box = (struct dialog_box*)data;
    
    menu_common_render_background(
        20, 140,
        280,
        80
    );

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 260,
            .height = 60,
            .wrap = WRAP_WORD,
        }, 
        1, 
        30, 150, 
        dialog_box->current_message_start,
        dialog_box->current_message_end - dialog_box->current_message_start
    );

    if (dialog_box->paused) {
        rspq_block_run(menu_icons_material->block);
        rdpq_texture_rectangle(
            TILE0,
            274, 196,
            290, 214,
            dialog_box->end_of_message ? 16 : 0, 0
        );
    }
}

void dialog_box_show(struct dialog_box* dialog_box, char* message, dialog_end_callback end_callback, void* end_callback_data) {
    dialog_box->current_message = message;
    menu_add_callback(dialog_box_render, dialog_box, 0);
    update_add(dialog_box, dialog_box_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_DIALOG);
    rdpq_text_register_font(1, dialog_box->font);

    dialog_box->current_message_start = message;
    dialog_box->current_message_end = message;

    dialog_box->requested_characters = 0.0f;
    dialog_box->paused = 0;
    dialog_box->end_of_message = 0;

    dialog_box->end_callback = end_callback;
    dialog_box->end_callback_data = end_callback_data;
}

void dialog_box_hide(struct dialog_box* dialog_box) {
    menu_remove_callback(dialog_box);
    update_remove(dialog_box);
}

void dialog_box_destroy(struct dialog_box* dialog_box) {
    font_cache_release(dialog_box->font);
}