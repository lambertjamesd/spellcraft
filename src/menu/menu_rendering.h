#ifndef __MENU_MENU_RENDERING_H__
#define __MENU_MENU_RENDERING_H__

typedef void (*menu_render_callback)(void* data);

void menu_add_callback(menu_render_callback callback, void* data);
void menu_remove_callback(menu_render_callback callback, void* data);

#endif