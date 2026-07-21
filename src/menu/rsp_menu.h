#ifndef __MENU_RSP_MENU_H__
#define __MENU_RSP_MENU_H__

#include <libdragon.h>
#include <stdint.h>
#include <stdbool.h>
#include "rsp/rsp_menu.inc"
#include "../math/vector2s16.h"
#include "rsp/rsp_menu_defs.h"

#define MENU_TO_FIXED(value)    (int16_t)(value * 4.0f)

enum menu_color_target {
    MENU_COLOR_TARGET_FILL = 0x37,
    MENU_COLOR_TARGET_FOG = 0x38,
    MENU_COLOR_TARGET_BLEND = 0x39,
    MENU_COLOR_TARGET_PRIM = 0x3A,
    MENU_COLOR_TARGET_ENV = 0x3B,
};

typedef enum menu_color_target menu_color_target_t;

struct transform_2d_fp {
    int16_t int_part[8];
    uint16_t frac_part[8];
} __attribute__((aligned(16)));

typedef struct transform_2d_fp transform_2d_fp_t;
typedef float transform_2d_t[6];

struct menu2d_line_vtx {
    vector2s16_t pos;
    int16_t u;
    uint16_t width;
    color_t color;
};

typedef struct menu2d_line_vtx menu2d_line_vtx_t;

struct menu2d_vtx {
    vector2s16_t pos;
    vector2s16_t uv;
    color_t color;
};

typedef struct menu2d_vtx menu2d_vtx_t;

void menu_transform_to_fixed(transform_2d_fp_t* output, const transform_2d_t input);

void menu_init();
void menu_teardown();

void menu_mtx(transform_2d_fp_t* mtx, bool mul, bool push);
void menu_mtx_pop(int count);

void menu_mtx_uv(transform_2d_fp_t* mtx, bool mul, bool push);
void menu_mtx_pop_uv(int count);

void menu_move_to(menu2d_line_vtx_t* vtx);
void menu_line_to(menu2d_line_vtx_t* vtx);
void menu_set_attr_flags(int flags);
void menu_set_viewport(int left, int top, int right, int bottom);
void menu_set_vtx_fx(int fx);

void menu_vtx(const menu2d_vtx_t* vtx, uint32_t offset, uint32_t count);

void menu_relative_fill_rect(int8_t vtx_index, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

void menu_relative_tex_rect(uint8_t vtx_index, uint8_t tile, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t s, int16_t t, int16_t dsdx, int16_t dtdy);

void menu_set_color_table(const color_t* table);
void menu_set_color(menu_color_target_t target, uint16_t index);

extern uint32_t MENU_OVERLAY_ID;

static inline void menu_tri(uint32_t a, uint32_t b, uint32_t c) {
    assert(a < MENU_TRI_VTX_COUNT && b < MENU_TRI_VTX_COUNT && c < MENU_TRI_VTX_COUNT);
    rspq_write(MENU_OVERLAY_ID, RSP_MENU_MenuCmd_Tri, (a << 16) | (b << 8) | c);
}

void* menu_get_state();

#endif