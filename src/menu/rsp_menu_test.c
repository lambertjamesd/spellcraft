#include "../test/framework_test.h"

#include "rsp_menu.h"

static uint32_t MENU_OVERLAY_ID = 0;

DEFINE_RSP_UCODE(rsp_menu);

#define MENU_CMD_VTX        0x1
#define MENU_CMD_LINE       0x2
#define MENU_CMD_MTX        0x3
#define MENU_CMD_MTX_POP    0x4
#define MENU_CMD_SET_STACK  0x5
#define MENU_CMD_TEST       0x6

static transform_2d_fp_t matrix_stack[10];

void test_menu_microcode(struct test_context* t) {
    MENU_OVERLAY_ID = rspq_overlay_register(&rsp_menu);

    transform_2d_t transform = {2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f};
    transform_2d_fp_t test_mtx;

    menu_transform_to_fixed(&test_mtx, transform);
    data_cache_hit_writeback(&test_mtx, sizeof(transform_2d_fp_t));

    rspq_write(MENU_OVERLAY_ID, MENU_CMD_SET_STACK, 10, PhysicalAddr(matrix_stack));
    rspq_write(MENU_OVERLAY_ID, MENU_CMD_MTX, MENU_MTX_MUL | MENU_MTX_PUSH, PhysicalAddr(&test_mtx));

    rspq_wait();

    uint32_t* data = rspq_overlay_get_state(&rsp_menu);
    debugf("%0d\n", (int)*data);
    
    rspq_overlay_unregister(MENU_OVERLAY_ID);
}