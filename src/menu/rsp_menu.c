#include "rsp_menu.h"

#include <libdragon.h>
#include "rsp/rsp_menu_defs.h"

#define MATRIX_STACK_SIZE       8
#define MAXTRIX_UV_STACK_SIZE   4

static uint32_t MENU_OVERLAY_ID = 0;
static transform_2d_fp_t matrix_stack[MATRIX_STACK_SIZE];
static transform_2d_fp_t matrix_stack_uv[MAXTRIX_UV_STACK_SIZE];

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
        rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_SetStack, (MAXTRIX_UV_STACK_SIZE << 8) | MATRIX_STACK_SIZE, PhysicalAddr(matrix_stack), PhysicalAddr(matrix_stack_uv));
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

void menu_mtx_uv(transform_2d_fp_t* mtx, bool mul, bool push) {
    int flags = MENU_MTX_UV;

    if (mul) {
        flags |= MENU_MTX_MUL;
    }

    if (push) {
        flags |= MENU_MTX_PUSH;
    }

    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_Mtx, flags, (int)mtx);
}

void menu_mtx_pop_uv(int count) {
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_MtxPop, 0x0100 | count);
}

void menu_point_action(int command, menu2d_line_vtx_t* vtx) {
    rspq_write(
        MENU_OVERLAY_ID,
        command,
        0,
        ((uint32_t)(uint16_t)vtx->pos.x << 16) | (uint32_t)(uint16_t)vtx->pos.y,
        ((uint32_t)vtx->u << 16) | (uint32_t)vtx->width,
        ((uint32_t)vtx->color.r << 24) | ((uint32_t)vtx->color.g << 16) | ((uint32_t)vtx->color.b << 8) | (uint32_t)vtx->color.a
    );
}

void menu_move_to(menu2d_line_vtx_t* vtx) {
    menu_point_action(RSP_MENU_MenuCmd_MoveTo, vtx);
}

void menu_line_to(menu2d_line_vtx_t* vtx) {
    menu_point_action(RSP_MENU_MenuCmd_LineTo, vtx);
}

void menu_set_attr_flags(int flags) {
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_SetAttrFlags, flags);
}

void menu_set_viewport(int left, int top, int right, int bottom) {
    rspq_write(
        MENU_OVERLAY_ID, 
        RSP_MENU_MenuCmd_SetViewport,
        0,
        ((uint32_t)left << 18) | (((uint32_t)-right << 2) & 0xFFFF),
        ((uint32_t)top << 18) | (((uint32_t)-bottom << 2) & 0xFFFF)
    );
}

void menu_set_vtx_fx(int fx) {
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_SetUVFx, fx);
}

void menu_vtx(const menu2d_vtx_t* vtx, uint32_t offset, uint32_t count) {
    assert(offset < MENU_TRI_VTX_COUNT);
    assert(offset + count < MENU_TRI_VTX_COUNT);
    
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_VTX, (offset << 8) | count, (int)vtx);
}

void* menu_get_state() {
    rspq_wait();
    return rspq_overlay_get_state(&rsp_menu);
}