#include "tutorial_menu.h"

#include "../time/time.h"
#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "../font/fonts.h"
#include "button_icons.h"
#include "../render/defs.h"
#include "../scene/scene.h"

static tutorial_menu_state_t tutorial_state;
static float tutorial_appear_timer;
uint16_t tutorial_text_width;

static material_pair_t* tut_overlay_material;
static rspq_block_t* tut_render_block;

static const char* tut_messages[] = {
    [TUTORIAL_MENU_STATE_NONE] = "",
    [TUTORIAL_MENU_CREATE_FIRE] = "Hold @z+@cr",
    [TUTORIAL_MENU_CAST] = "Cast @a",
};

#define ICON_SIZE      16
#define BOX_PADDING     2

#define BOX_Y           160

#define GROW_TIME       0.125f
#define ANIM_TIME       0.4f

void tutorial_render(void* data) {
    if (tutorial_appear_timer < GROW_TIME) {
        int x = (SCREEN_WD - tutorial_text_width) >> 1;
        rspq_block_run(tut_overlay_material->apply.block);
        rdpq_set_prim_register_raw((color_t){255, 255, 255, 255}, 0, 0);
        
        float lerp = tutorial_appear_timer * (1.0f / GROW_TIME);

        int offset = ((1.0f - lerp) * ((ICON_SIZE + BOX_PADDING * 2) * 0.5f));

        rdpq_fill_rectangle(x - BOX_PADDING, BOX_Y - BOX_PADDING + offset, x + tutorial_text_width + BOX_PADDING, BOX_Y + ICON_SIZE + BOX_PADDING - offset);
        return;
    }

    rspq_block_run(tut_render_block);

    if (tutorial_appear_timer < ANIM_TIME) {
        int x = (SCREEN_WD - tutorial_text_width) >> 1;

        float lerp = (tutorial_appear_timer - GROW_TIME) * (1.0f / (ANIM_TIME - GROW_TIME));

        rspq_block_run(tut_overlay_material->apply.block);
        rdpq_set_prim_register_raw((color_t){255, 255, 255, 255 - (uint8_t)(lerp * 255.0f)}, 0, 0);

        rdpq_fill_rectangle(x - BOX_PADDING, BOX_Y - BOX_PADDING, x + tutorial_text_width + BOX_PADDING, BOX_Y + ICON_SIZE + BOX_PADDING);
    }
}

void tutorial_update(void* data) {
    switch (tutorial_state) {
        case TUTORIAL_MENU_STATE_NONE:
            break;
        case TUTORIAL_MENU_CREATE_FIRE:
            if (
                live_cast_has_spell(&current_scene->player.live_cast, 
                SPELL_SYMBOL_FIRE,
                false, false, false, false)
            ) {
                tutorial_set_step(TUTORIAL_MENU_CAST);
            }
            break;
        case TUTORIAL_MENU_CAST:
            if (current_scene->player.live_cast.was_cast) {
                tutorial_set_step(TUTORIAL_MENU_STATE_NONE);
            }
            break;
        default:
            assert(false);
    }

    if (tutorial_appear_timer < ANIM_TIME) {
        tutorial_appear_timer += fixed_time_step;
    }
}

int tutorial_read_button_offset(const char** curr_ptr) {
    const char* curr = *curr_ptr;
    *curr_ptr += 1;

    switch (*curr) {
        case 'u':
            return 0;
        case 'd':
            return 1;
        case 'l':
            return 2;
        case 'r':
            return 3;
        default:
            debugf("%c\n", *curr);
            assert(false);
    }
}

enum button_type tutorial_read_button_type(const char** curr_ptr) {
    const char* curr = *curr_ptr;

    if (*curr != '@') {
        return BUTTON_TYPE_A;
    }

    curr += 1;
    *curr_ptr = curr + 1;

    switch (*curr) {
        case 'a':
            return BUTTON_TYPE_A;
        case 'b':
            return BUTTON_TYPE_B;
        case 'z':
            return BUTTON_TYPE_Z;
        case 's':
            return BUTTON_TYPE_START;
        case 'd':
            return BUTTON_TYPE_D_U + tutorial_read_button_offset(curr_ptr);
        case 'l':
            return BUTTON_TYPE_L;
        case 'r':
            return BUTTON_TYPE_R;
        case 'c':
            return BUTTON_TYPE_C_U + tutorial_read_button_offset(curr_ptr);
        default:
            debugf("%c\n", *curr);
            assert(false);
    }
}

void tutorial_destroy_step() {
    if (!tutorial_state) {
        return;
    }

    const char* message = tut_messages[tutorial_state];

    while (*message) {
        if (*message == '@') {
            button_icon_unload(tutorial_read_button_type(&message));
        } else {
            message += 1;
        }
    }

    rspq_call_deferred((void (*)(void *))rspq_block_free, tut_render_block);
}

void tutorial_init_step() {
    tutorial_text_width = 0;
    const char* message = tut_messages[tutorial_state];
    const char* last_start = message;

    while (*message) {
        if (*message == '@') {
            tutorial_text_width += ICON_SIZE;
            tutorial_text_width += measure_text(FONT_DIALOG, last_start, message - last_start);

            button_icon_load(tutorial_read_button_type(&message));

            last_start = message;
        } else {
            message += 1;
        }
    }
    
    tutorial_text_width += measure_text(FONT_DIALOG, last_start, message - last_start);

    message = tut_messages[tutorial_state];
    last_start = message;

    int x = (SCREEN_WD - tutorial_text_width) >> 1;

    rspq_block_begin();

    rspq_block_run(tut_overlay_material->apply.block);
    rdpq_set_prim_register_raw((color_t){0, 0, 0, 128}, 0, 0);

    rdpq_fill_rectangle(x - BOX_PADDING, BOX_Y - BOX_PADDING, x + tutorial_text_width + BOX_PADDING, BOX_Y + ICON_SIZE + BOX_PADDING);

    while (*message) {
        if (*message == '@') {
            rdpq_textmetrics_t metrics = rdpq_text_printn(
                &(rdpq_textparms_t){
                    // .line_spacing = -3,
                    .align = ALIGN_LEFT,
                    .valign = VALIGN_BOTTOM,
                    .width = tutorial_text_width,
                    .height = 0,
                    .wrap = WRAP_NONE,
                }, 
                FONT_DIALOG, 
                x, BOX_Y + 12, 
                last_start,
                message - last_start
            );

            x += metrics.advance_x;

            button_icon_draw(tutorial_read_button_type(&message), x, BOX_Y);
            

            last_start = message;

            x += ICON_SIZE;
        } else {
            message += 1;
        }
    }
    
    rdpq_textmetrics_t metrics = rdpq_text_printn(
        &(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_BOTTOM,
            .width = tutorial_text_width,
            .height = 0,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        x, BOX_Y + 12, 
        last_start,
        message - last_start
    );

    tut_render_block = rspq_block_end();
}

void tutorial_cancel() {
    if (!tutorial_state) {
        return;
    }

    material_cache_release(tut_overlay_material);
    tut_overlay_material = NULL;
    tutorial_state = TUTORIAL_MENU_STATE_NONE;

    update_remove(&tutorial_state);
    menu_remove_callback(&tutorial_state);
    font_type_release(FONT_DIALOG);
}

void tutorial_set_step(tutorial_menu_state_t state) {
    assert(state >= 0 && state < TUTORIAL_STATE_COUNT);

    if (state == TUTORIAL_MENU_STATE_NONE) {
        tutorial_cancel();
        return;
    }

    tutorial_appear_timer = 0.0f;

    if (state == tutorial_state) {
        return;
    }

    if (tutorial_state == TUTORIAL_MENU_STATE_NONE) {
        tut_overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
        update_add(&tutorial_state, tutorial_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD | UPDATE_LAYER_PAUSE_MENU);
        menu_add_callback(tutorial_render, &tutorial_state, MENU_PRIORITY_HUD);
        font_type_use(FONT_DIALOG);
    } else {
        tutorial_destroy_step();
    }

    tutorial_state = state;
    
    tutorial_init_step();
}

bool tutorial_is_running() {
    return tutorial_state != TUTORIAL_MENU_STATE_NONE;
}