#include "show_rune.h"

#include "../time/time.h"
#include "../menu/menu_common.h"
#include "../math/mathf.h"
#include "../resource/material_cache.h"

#define RUNE_SHOW_TIME              3.0f
#define RUNE_FADE_TIME              0.25f
#define RUNE_FLASH_TIME             2.0f

void show_init(struct show_rune* show_rune) {
    show_rune->spell_symbol_material = material_cache_load("rom:/materials/spell/symbols.mat");
    show_rune->should_show = 0;
    show_rune->show_rune_timer = 0.0f;
    show_rune->showing_rune = 0;
}

void show_rune_start(struct show_rune* show_rune, union cutscene_step_data* data) {
    show_rune->should_show = data->show_rune.should_show;

    if (data->show_rune.should_show) {
        show_rune->showing_rune = data->show_rune.rune;
        show_rune->show_rune_timer = 0.0f;
    } else {
        show_rune->show_rune_timer = RUNE_FADE_TIME;
    }
}

bool show_rune_update(struct show_rune* show_rune, union cutscene_step_data* data) {
    float target = data->show_rune.should_show ? RUNE_SHOW_TIME : 0.0f;
    show_rune->show_rune_timer = mathfMoveTowards(show_rune->show_rune_timer, target, fixed_time_step);

    joypad_buttons_t buttons = joypad_get_buttons_pressed(0);

    if (buttons.start) {
        show_rune->show_rune_timer = target;
    }

    return show_rune->show_rune_timer == target;
}

void show_rune_runder(struct show_rune* show_rune) {
    if (show_rune->showing_rune && show_rune->show_rune_timer) {
        rspq_block_run(solid_primitive_material->block);

        float alpha = show_rune->show_rune_timer * (1.0f / RUNE_FADE_TIME);

        if (alpha > 1.0f) {
            alpha = 1.0f;
        }

        color_t color = {0, 0, 0, (uint8_t)(alpha * 128.0f)};
        rdpq_set_prim_color(color);

        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);

        int size = (int)(32.0f * alpha);

        rspq_block_run(current_spell_icon->block);
        rdpq_texture_rectangle_scaled(
            TILE0, 
            160 - size, 80 - size,
            160 + size, 80 + size,
            0, 0,
            32, 32
        );

        if (!show_rune->showing_rune || show_rune->show_rune_timer > RUNE_FADE_TIME) {
            rspq_block_run(show_rune->spell_symbol_material->block);

            int offset = (show_rune->showing_rune - 1) * 24;

            if (!show_rune->showing_rune) {
                color_t color = {255, 255, 255, (uint8_t)(alpha * 255.0f)};
                rdpq_set_prim_color(color);
            }

            rdpq_texture_rectangle_scaled(
                TILE0, 
                160 - 24, 80 - 24,
                160 + 24, 80 + 24,
                offset, 0,
                24 + offset, 24
            );
        }

        float flash_time = show_rune->show_rune_timer - RUNE_FADE_TIME;

        if (flash_time > 0.0f && flash_time < RUNE_FLASH_TIME) {
            float alpha = 1.0f - (flash_time * (1.0f / RUNE_FLASH_TIME));
            alpha *= alpha;

            rspq_block_run(solid_primitive_material->block);
            color_t color = {255, 255, 255, (uint8_t)(alpha * 255.0f)};
            rdpq_set_prim_color(color);
            rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
        }
    }
}