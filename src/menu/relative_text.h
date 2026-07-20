#ifndef __MENU_RELATIVE_TEXT_H__
#define __MENU_RELATIVE_TEXT_H__

#include <libdragon.h>
#include "rsp_menu.h"

void rsp_menu_render_paragraph(const rdpq_paragraph_t *layout, float x0, float y0, int relative_vtx);

#endif