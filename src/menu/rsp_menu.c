#include "rsp_menu.h"

#include <libdragon.h>
#include "rsp/rsp_menu_defs.h"

static uint32_t MENU_OVERLAY_ID = 0;
static transform_2d_fp_t matrix_stack[10];

DEFINE_RSP_UCODE(rsp_menu);

#define F32_TO_FIXED(val) (int32_t)((val) * (float)(1<<16))

void menu_transform_to_fixed(transform_2d_fp_t* output, const transform_2d_t input) {
    int16_t* int_part = output->int_part;
    uint16_t* frac_part = output->frac_part;
    const float* input_ptr = input;
    
    for (int i = 0; i < 2; i += 1) {
        uint32_t fixed_a = F32_TO_FIXED(input_ptr[0]);
        uint32_t fixed_b = F32_TO_FIXED(input_ptr[1]);
        uint32_t fixed_c = F32_TO_FIXED(input_ptr[2]);

        *(uint64_t*)int_part = 
            ((uint64_t)(fixed_a & 0xFFFF0000) << 32) | 
            ((uint64_t)(fixed_b & 0xFFFF0000) << 16) | 
            (uint64_t)(fixed_c & 0xFFFF0000);
        
        *(uint64_t*)frac_part = 
            ((uint64_t)(fixed_a & 0xFFFF) << 48) | 
            ((uint64_t)(fixed_b & 0xFFFF) << 32) | 
            ((uint64_t)(fixed_c & 0xFFFF) << 16);

        int_part += 4;
        frac_part += 4;
        input_ptr += 3;
    }
}

void menu_init() {
    if (!MENU_OVERLAY_ID) {
        MENU_OVERLAY_ID = rspq_overlay_register(&rsp_menu);
        rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_SetStack, 10, PhysicalAddr(matrix_stack));
    }
}

void menu_teardown() {
    if (MENU_OVERLAY_ID) {
        rspq_overlay_unregister(MENU_OVERLAY_ID);
        MENU_OVERLAY_ID = 0;
    }
}

void menu_mtx(transform_2d_fp_t* mtx, bool mul, bool push) {
    int flags = 0;

    if (mul) {
        flags |= MENU_MTX_MUL;
    }

    if (push) {
        flags |= MENU_MTX_PUSH;
    }

    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_Mtx, flags, (int)mtx);
}

void menu_mtx_pop(int count) {
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_MtxPop, count);
}

void menu_point_action(int command, vector2s16_t* pos, uint16_t u, uint16_t w, color_t color) {
    rspq_write(
        MENU_OVERLAY_ID,
        command,
        0,
        ((uint32_t)pos->x << 16) | (uint32_t)pos->y,
        ((uint32_t)u << 16) | (uint32_t)w,
        ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | (uint32_t)color.a
    );
}

void menu_move_to(vector2s16_t* pos, uint16_t u, uint16_t w, color_t color) {
    menu_point_action(RSP_MENU_MenuCmd_MoveTo, pos, u, w, color);
}

void* menu_get_state() {
    rspq_wait();
    return rspq_overlay_get_state(&rsp_menu);
}