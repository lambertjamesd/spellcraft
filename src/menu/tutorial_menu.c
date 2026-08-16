#include "tutorial_menu.h"

#include "../time/time.h"
#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "../font/fonts.h"
#include "button_icons.h"
#include "../render/defs.h"

static tutorial_menu_state_t tutorial_state;

static material_pair_t* tut_overlay_material;
static rspq_block_t* tut_render_block;

static const char* tut_messages[] = {
    [TUTORAIL_MENU_STATE_NONE] = "",
    [TUTORAIL_MENU_CREATE_FIRE] = "Hold @z + @cr",
    [TUTORAIL_MENU_CAST] = "Cast @a",
};

#define ICON_SIZE      16
#define ICON_PADDING    1
#define BOX_PADDING     2

#define BOX_Y           160

void tutorial_render(void* data) {
    rspq_block_run(tut_render_block);
}

void tutorial_update(void* data) {
    
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
    int width = 0;
    const char* message = tut_messages[tutorial_state];
    const char* last_start = message;

    while (*message) {
        if (*message == '@') {
            if (width) {
                width += ICON_PADDING;
            }

            width += ICON_SIZE;
            width += measure_text(FONT_DIALOG, last_start, message - last_start);

            button_icon_load(tutorial_read_button_type(&message));

            if (*message) {
                width += ICON_PADDING;
            }

            last_start = message;
        } else {
            message += 1;
        }
    }
    
    width += measure_text(FONT_DIALOG, last_start, message - last_start);

    message = tut_messages[tutorial_state];
    last_start = message;

    int x = (SCREEN_WD - width) >> 1;

    rspq_block_begin();

    rspq_block_run(tut_overlay_material->apply.block);
    rdpq_set_prim_color((color_t){0, 0, 0, 128});

    rdpq_fill_rectangle(x - BOX_PADDING, BOX_Y - BOX_PADDING, x + width + BOX_PADDING, BOX_Y + ICON_SIZE + BOX_PADDING);

    while (*message) {
        if (*message == '@') {
            rdpq_textmetrics_t metrics = rdpq_text_printn(
                &(rdpq_textparms_t){
                    // .line_spacing = -3,
                    .align = ALIGN_LEFT,
                    .valign = VALIGN_BOTTOM,
                    .width = width,
                    .height = 0,
                    .wrap = WRAP_NONE,
                }, 
                FONT_DIALOG, 
                x, BOX_Y + 12, 
                last_start,
                message - last_start
            );

            x += metrics.advance_x;
            x += ICON_PADDING;

            button_icon_draw(tutorial_read_button_type(&message), x, BOX_Y);
            

            last_start = message;

            x += ICON_SIZE;

            if (*message) {
                x += ICON_PADDING;
            }
        } else {
            message += 1;
        }
    }
    
    rdpq_textmetrics_t metrics = rdpq_text_printn(
        &(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_BOTTOM,
            .width = width,
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
    tutorial_state = TUTORAIL_MENU_STATE_NONE;

    update_remove(&tutorial_state);
    menu_remove_callback(&tutorial_state);
    font_type_release(FONT_DIALOG);
}

void tutorial_set_step(tutorial_menu_state_t state) {
    if (state == TUTORAIL_MENU_STATE_NONE) {
        tutorial_cancel();
        return;
    }

    if (tutorial_state == TUTORAIL_MENU_STATE_NONE) {
        tut_overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
        update_add(&tutorial_state, tutorial_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD);
        menu_add_callback(tutorial_render, &tutorial_state, MENU_PRIORITY_HUD);
        font_type_use(FONT_DIALOG);
    } else {
        tutorial_destroy_step();
    }

    tutorial_state = state;
    
    tutorial_init_step();
}
