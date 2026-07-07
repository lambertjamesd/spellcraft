#include "../test/framework_test.h"

#include "rsp_menu.h"
#include "rsp/rsp_menu_defs.h"

void test_menu_microcode(struct test_context* t) {
    menu_init();

    transform_2d_t transform = {2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f};
    transform_2d_fp_t test_mtx;

    menu_transform_to_fixed(&test_mtx, transform);
    data_cache_hit_writeback(&test_mtx, sizeof(transform_2d_fp_t));

    menu_mtx((transform_2d_fp_t*)PhysicalAddr(&test_mtx), true, true);
    
    uint8_t* data = menu_get_state();
    uint16_t mtx[8];

    memcpy(mtx, data + RSP_MENU_MTX_TOP, 16);

    test_eqi(t, 2, mtx[0]);
    test_eqi(t, 0, mtx[1]);
    test_eqi(t, 0, mtx[2]);
    test_eqi(t, 0, mtx[4]);
    test_eqi(t, 2, mtx[5]);
    test_eqi(t, 0, mtx[6]);
    
    menu_move_to(&(vector2s16_t){{{2, 5}}}, 0, 0, (color_t){});

    menu_mtx_pop(1);

    data = menu_get_state();
    data_cache_hit_invalidate(data + RSP_MENU_MTX_TOP, 16);
    memcpy(mtx, data + RSP_MENU_MTX_TOP, 16);

    test_eqi(t, 1, mtx[0]);
    test_eqi(t, 0, mtx[1]);
    test_eqi(t, 0, mtx[2]);
    test_eqi(t, 0, mtx[4]);
    test_eqi(t, 1, mtx[5]);
    test_eqi(t, 0, mtx[6]);
    
    menu_teardown();
}