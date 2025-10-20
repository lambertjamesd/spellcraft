#ifndef __MENU_LIVE_CAST_RENDERER_H_
#define __MENU_LIVE_CAST_RENDERER_H_

#include "../spell/live_cast.h"
#include "../render/material.h"
#include "../render/render_batch.h"

struct symbol_modifier_parameters {
    uint8_t background_alpha;
    uint8_t rune_alpha;
    int8_t x_offset;
    int8_t y_offset;
    uint8_t size;
};

struct live_cast_renderer {
    live_cast_t* live_cast;
    material_t* icon_background;
    material_t* spell_icons;

    struct symbol_modifier_parameters symbol_modifiers[4];
};

typedef struct live_cast_renderer live_cast_renderer_t;

void live_cast_renderer_init(live_cast_renderer_t* live_cast_renderer, live_cast_t* live_cast);
void live_cast_renderer_destroy(live_cast_renderer_t* live_cast_renderer);

void live_cast_renderer_render(live_cast_renderer_t* live_cast_renderer);
void live_cast_renderer_update(live_cast_renderer_t* live_cast_renderer);

#endif