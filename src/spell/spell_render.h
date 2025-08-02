#ifndef __SPELL_SPELL_RENDER_H__
#define __SPELL_SPELL_RENDER_H__

#include "spell.h"

struct spell_render_animation {
    float last_symbol_time;
};

void spell_render(struct spell* spell, int x, int y, struct spell_render_animation* animation);
void spell_render_icon(enum inventory_item_type type, int x, int y);

#endif