#ifndef __MENU_BUTTON_ICONS_H__
#define __MENU_BUTTON_ICONS_H__

enum button_type {
    BUTTON_TYPE_A,
    BUTTON_TYPE_B,
    BUTTON_TYPE_Z,
    BUTTON_TYPE_START,
    BUTTON_TYPE_D_U,
    BUTTON_TYPE_D_D,
    BUTTON_TYPE_D_L,
    BUTTON_TYPE_D_R,
    BUTTON_TYPE_Y,
    BUTTON_TYPE_X,
    BUTTON_TYPE_L,
    BUTTON_TYPE_R,
    BUTTON_TYPE_C_U,
    BUTTON_TYPE_C_D,
    BUTTON_TYPE_C_L,
    BUTTON_TYPE_C_R,
};

void button_icon_load(enum button_type type);
void button_icon_unload(enum button_type type);

void button_icon_draw(enum button_type type, int x, int y);

#endif