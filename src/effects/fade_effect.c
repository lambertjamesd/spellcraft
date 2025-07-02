#include "fade_effect.h"

#include <stdbool.h>
#include "../time/time.h"
#include "../menu/menu_rendering.h"
#include "../menu/menu_common.h"
#include "../render/defs.h"

static struct Coloru8 start_color;
static struct Coloru8 end_color;
static float lerp_time;
static float current_time;
static bool is_active;

struct Coloru8 fade_effect_calculate_color() {
    if (current_time <= 0.0f) {
        return start_color;
    }

    if (current_time >= lerp_time) {
        return end_color;
    }

    return coloru8_lerp(&start_color, &end_color, current_time / lerp_time);
}

void fade_effect_update(void* data) {
    if (current_time < lerp_time) {
        current_time += fixed_time_step;
    }
}

void fade_effect_render(void* data) {
    rspq_block_run(solid_primitive_material->block);

    struct Coloru8 color = fade_effect_calculate_color();

    rdpq_set_prim_color((color_t){color.r, color.g, color.b, color.a});
    rdpq_texture_rectangle(
        TILE0, 
        0, 0,
        320, 240,
        1, 1
    );
}

void fade_effect_activate() {
    if (is_active) {
        return;
    }
    is_active = true;

    update_add(&start_color, fade_effect_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD);
    menu_add_callback(fade_effect_render, &start_color, MENU_PRIORITY_OVERLAY);
}

void fade_effect_set(struct Coloru8 color, float time) {
    start_color = fade_effect_calculate_color();
    if (start_color.a == 0) {
        start_color = color;
        start_color.a = 0;
    }
    if (color.a == 0) {
        end_color = start_color;
        end_color.a = 0;
    } else {
        end_color = color;
    }
    lerp_time = time;
    current_time = 0.0f;

    fade_effect_activate();
}