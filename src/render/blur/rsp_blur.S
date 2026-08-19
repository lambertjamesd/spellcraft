
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand BlurCmd_Blur, 8
    RSPQ_EndOverlayHeader
    RSPQ_EmptySavedState
.bss
    .align 4
    SCREEN_BUFFER: .space 2 * 8 * 122

.text
    
    #define var         $v01
    #define vag         $v02
    #define vab         $v03
    #define vbr         $v04
    #define vbg         $v05
    #define vbb         $v06
    #define vcr         $v07
    #define vcg         $v08
    #define vcb         $v09
    #define voutr       $v10
    #define voutg       $v11
    #define voutb       $v12

    #define vw_center   $v13.e0
    #define vw_side     $v13.e1
    
    #define vr_mask     $v13.e2
    #define vg_mask     $v13.e3
    #define vb_mask     $v13.e4
    
    #define vr_round    $v13.e5
    #define vg_round    $v13.e6
    #define vb_round    $v13.e7
    
    #define vtmp        $v14

#define CAT_RAW(a, b) a ## b
#define CAT(a, b) CAT_RAW(a, b)

#define VREG(c, channel) CAT(v, CAT(c, channel))

#define blur_mul(x, y, z) \
    vmudl voutr, VREG(x, r), vw_side; \
    vmadl voutr, VREG(y, r), vw_center; \
    vmadl voutr, VREG(z, r), vw_side; \
    \
    vmudl voutg, VREG(x, g), vw_side; \
    vmadl voutg, VREG(y, g), vw_center; \
    vmadl voutg, VREG(z, g), vw_side; \
    \
    vadd voutr, vr_round; \
    \
    vmudl voutb, VREG(x, b), vw_side; \
    vmadl voutb, VREG(y, b), vw_center; \
    vmadl voutb, VREG(z, b), vw_side; \
    \
    vadd voutg, vg_round; \
    vadd voutb, vb_round; \
    \
    vand voutr, voutr, vr_mask; \
    vand voutg, voutg, vg_mask; \
    vand voutb, voutb, vb_mask

.macro recombine_output
    # recombine channels
    vor voutr, voutr, voutg
    vor voutr, voutr, voutb
.endm

#define mask_color(x) \
    vand VREG(x, b), VREG(x, r), vb_mask; \
    vand VREG(x, g), VREG(x, r), vg_mask; \
    vand VREG(x, r), VREG(x, r), vr_mask


#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

    ##########################################
    # BlurCmd_Blur
    # 
    # a0: blur_strength
    # a1: screen_buffer
BlurCmd_Blur:
    #define blur_strength   a0
    #define screen_buffer   a1

    #define buffer_ptr      t2
    #define buffer_ptr_end  t3
    #define tmp             t4
    #define pos_y           t5
    #define pos_x           t6
    
    andi blur_strength, 0xFFFF
    mtc2 blur_strength, vw_center

    li tmp, 0x10000
    sub tmp, tmp, blur_strength
    srl tmp, tmp, 1
    mtc2 tmp, vw_side

    li tmp, 0xF800
    mtc2 tmp, vr_mask
    li tmp, 0x7C0
    mtc2 tmp, vg_mask
    li tmp, 0x3E
    mtc2 tmp, vb_mask
    
    li tmp, 0x0400
    mtc2 tmp, vr_round
    li tmp, 0x020
    mtc2 tmp, vg_round
    li tmp, 0x1
    mtc2 tmp, vb_round

    li pos_x, (320 - 8) * 2
    
BlurCmd_Blur_ColLoopPoint:
    li pos_y, 0

    li dma_size, DMA_SIZE(16, 122)
    li row_stride, 640
    add dram_address, screen_buffer, pos_x
    jal DMAIn
    li dmem_address, %lo(SCREEN_BUFFER)

    li buffer_ptr, %lo(SCREEN_BUFFER)
    addi buffer_ptr_end, buffer_ptr, 2 * 8 * 120

    # load first 3 rows
    lqv var, 0, 0, buffer_ptr
    lqv vbr, 0, 0, buffer_ptr
    lqv vcr, 0, 16, buffer_ptr

    # separate channels
    mask_color(a)
    mask_color(b)
    mask_color(c)
BlurCmd_Blur_BatchLoopPoint:
    blur_mul(a, b, c)

    # load next row
    lqv var, 0, 32, buffer_ptr
    
    recombine_output

    # mask next
    mask_color(a)

    # store recombined result
    sqv voutr, 0, 0, buffer_ptr

    blur_mul(b, c, a)

    # load next row
    lqv vbr, 0, 48, buffer_ptr
    
    recombine_output
    
    # mask next
    mask_color(b)

    # store recombined result
    sqv voutr, 0, 16, buffer_ptr
    
    blur_mul(c, a, b)
    
    # load next row
    lqv vcr, 0, 64, buffer_ptr
    
    recombine_output

    # mask next
    mask_color(c)
    
    # store recombined result
    sqv voutr, 0, 32, buffer_ptr

    addi buffer_ptr, 48
    blt buffer_ptr, buffer_ptr_end, BlurCmd_Blur_BatchLoopPoint
    nop
BlurCmd_Blur_BatchLoopEnd:
    # save last result
    jal DMAOut
    nop

    bne pos_y, zero, BlurCmd_Blur_ColEnd
    nop
    
    li dma_size, DMA_SIZE(16, 120)
    li row_stride, 640
    li tmp, 320 * 120 * 2
    add dram_address, screen_buffer, tmp
    add dram_address, pos_x
    jal DMAIn
    li dmem_address, %lo(SCREEN_BUFFER)

    li buffer_ptr, %lo(SCREEN_BUFFER)
    addi buffer_ptr_end, buffer_ptr, 2 * 8 * 120

    # duplicate last row
    lqv vtmp, 0, -16, buffer_ptr_end
    sqv vtmp, 0, 0, buffer_ptr_end

    j BlurCmd_Blur_BatchLoopPoint
    addi pos_y, 1
BlurCmd_Blur_ColEnd:
    bne pos_x, zero, BlurCmd_Blur_ColLoopPoint
    addi pos_x, -8 * 2

    BlurCmd_Blur_ColLoopEnd:

    j RSPQ_Loop
    nop

