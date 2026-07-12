#ifndef __MENU_RSP_MENU_H__
#define __MENU_RSP_MENU_H__

#include <libdragon.h>
#include <stdint.h>
#include <stdbool.h>
#include "rsp/rsp_menu.inc"
#include "../math/vector2s16.h"

struct transform_2d_fp {
    int16_t int_part[8];
    uint16_t frac_part[8];
} __attribute__((aligned(16)));

typedef struct transform_2d_fp transform_2d_fp_t;
typedef float transform_2d_t[6];

struct menu2d_vtx {
    vector2s16_t pos;
    int16_t u;
    uint16_t width;
    color_t color;
};

typedef struct menu2d_vtx menu2d_vtx_t;

void menu_transform_to_fixed(transform_2d_fp_t* output, const transform_2d_t input);

void menu_init();
void menu_teardown();

void menu_mtx(transform_2d_fp_t* mtx, bool mul, bool push);
void menu_mtx_pop(int count);

void menu_move_to(menu2d_vtx_t* vtx);
void menu_line_to(menu2d_vtx_t* vtx);
void menu_set_attr_flags(int flags);
void menu_set_viewport(int left, int right, int top, int bottom);

void* menu_get_state();

#endif