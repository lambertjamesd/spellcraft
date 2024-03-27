#ifndef __MENU_DIALOG_BOX_H__
#define __MENU_DIALOG_BOX_H__

#include <libdragon.h>

typedef void (*dialog_end_callback)(void* data);

#define MAX_MESSAGE_CHARACTERS  256

struct dialog_box {
    char* current_message;
    char current_text[MAX_MESSAGE_CHARACTERS];
    rdpq_font_t* font;

    char* current_message_start;
    char* current_message_end;
    float requested_characters;

    uint16_t paused: 1;
    uint16_t end_of_message: 1;

    dialog_end_callback end_callback;
    void* end_callback_data;
};

extern struct dialog_box g_dialog_box;

void dialog_box_init(struct dialog_box* dialog_box);
void dialog_box_show(struct dialog_box* dialog_box, char* message, dialog_end_callback end_callback, void* end_callback_data);
void dialog_box_hide(struct dialog_box* dialog_box);
void dialog_box_destroy(struct dialog_box* dialog_box);

#endif