#include "area_title.h"

#include <libdragon.h>
#include "../resource/font_cache.h"
#include "../time/time.h"
#include "../menu/menu_rendering.h"

#define MAX_TITLE_LEN   16
#define TITLE_SHOW_TIME 4.0f
#define TITLE_FADE_TIME 1.0f

struct area_title {
    rdpq_font_t* font;
    char message[16];
    float time_left;
};

static struct area_title g_title;

void area_title_render(void* data) {
    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_CENTER,
            .valign = VALIGN_TOP,
            .width = 320,
            .height = 60,
            .wrap = WRAP_WORD,
        }, 
        2, 
        0, 160, 
        g_title.message,
        strlen(g_title.message)
    );
}

void area_title_update(void* data) {
    g_title.time_left -= fixed_time_step;

    if (g_title.time_left <= 0.0f) {
        area_title_hide();
    }
}

void area_title_show(const char* title) {
    strncpy(g_title.message, title, MAX_TITLE_LEN);
    g_title.message[MAX_TITLE_LEN - 1] = '\0';
    g_title.time_left = TITLE_SHOW_TIME;

    if (g_title.font) {
        return;
    }

    g_title.font = font_cache_load("rom:/fonts/HeavyEquipment.font64");
    rdpq_text_register_font(2, g_title.font);
    menu_add_callback(area_title_render, &g_title, MENU_PRIORITY_TITLE);
}

void area_title_hide() {
    g_title.time_left = 0;

    if (g_title.font) {
        font_cache_release(g_title.font);
        g_title.font = NULL;
        menu_remove_callback(&g_title);
    }
}