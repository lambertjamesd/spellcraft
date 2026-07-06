#ifndef __MENU_RSP_MENU_H__
#define __MENU_RSP_MENU_H__

#include <stdint.h>
#include "rsp/rsp_menu.inc"

struct transform_2d_fp {
    int16_t int_part[8];
    uint16_t frac_part[8];
} __attribute__((aligned(16)));

typedef struct transform_2d_fp transform_2d_fp_t;
typedef float transform_2d_t[6];

void menu_transform_to_fixed(transform_2d_fp_t* output, const transform_2d_t input);

#endif