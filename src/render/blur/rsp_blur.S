
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
    
    li dma_size, DMA_SIZE(16, 122)
    li row_stride, 640
    move dram_address, screen_buffer
    jal DMAIn
    li dmem_address, %lo(SCREEN_BUFFER)
    
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

    li buffer_ptr, %lo(SCREEN_BUFFER)
    addi buffer_ptr_end, buffer_ptr, 2 * 8 * 120

    li pos_y, 0

    # load first 3 rows
    lqv var, 0, 0, buffer_ptr
    lqv vbr, 0, 0, buffer_ptr
    lqv vcr, 0, 16, buffer_ptr

    # separate channels
    vand vab, var, vb_mask
    vand vag, var, vg_mask
    vand var, var, vr_mask
    
    vand vbb, vbr, vb_mask
    vand vbg, vbr, vg_mask
    vand vbr, vbr, vr_mask
    
    vand vcb, vcr, vb_mask
    vand vcg, vcr, vg_mask
    vand vcr, vcr, vr_mask
BlurCmd_Blur_BatchLoopPoint:
    # out = a * edge + b * center + c * edge
    vmudl voutr, var, vw_side
    vmadl voutr, vbr, vw_center
    vmadl voutr, vcr, vw_side
    
    vmudl voutg, vag, vw_side
    vmadl voutg, vbg, vw_center
    vmadl voutg, vcg, vw_side

    # round red
    vadd voutr, vr_round
    
    vmudl voutb, vab, vw_side
    vmadl voutb, vbb, vw_center
    vmadl voutb, vcb, vw_side
    
    # round green blue
    vadd voutg, vg_round
    vadd voutb, vb_round

    # mask result
    vand voutr, voutr, vr_mask
    vand voutg, voutg, vg_mask
    vand voutb, voutb, vb_mask

    # load next row
    lqv var, 0, 32, buffer_ptr
    
    # recombine channels
    vor voutr, voutr, voutg
    vor voutr, voutr, voutb

    # mask next
    vand vab, var, vb_mask
    vand vag, var, vg_mask
    vand var, var, vr_mask

    # store recombined result
    sqv voutr, 0, 0, buffer_ptr

    # out = b * edge + c * center + a * edge
    vmudl voutr, vbr, vw_side
    vmadl voutr, vcr, vw_center
    vmadl voutr, var, vw_side
    
    vmudl voutg, vbg, vw_side
    vmadl voutg, vcg, vw_center
    vmadl voutg, vag, vw_side

    # round red
    vadd voutr, vr_round
    
    vmudl voutb, vbb, vw_side
    vmadl voutb, vcb, vw_center
    vmadl voutb, vab, vw_side
    
    # round green blue
    vadd voutg, vg_round
    vadd voutb, vb_round

    # mask result
    vand voutr, voutr, vr_mask
    vand voutg, voutg, vg_mask
    vand voutb, voutb, vb_mask

    # load next row
    lqv vbr, 0, 48, buffer_ptr
    
    # recombine channels
    vor voutr, voutr, voutg
    vor voutr, voutr, voutb
    
    # mask next
    vand vbb, vbr, vb_mask
    vand vbg, vbr, vg_mask
    vand vbr, vbr, vr_mask

    # store recombined result
    sqv voutr, 0, 16, buffer_ptr
    
    # out = c * edge + a * center + b * edge
    vmudl voutr, vcr, vw_side
    vmadl voutr, vbr, vw_center
    vmadl voutr, var, vw_side
    
    vmudl voutg, vcg, vw_side
    vmadl voutg, vbg, vw_center
    vmadl voutg, vag, vw_side
    
    # round red
    vadd voutr, vr_round

    vmudl voutb, vcb, vw_side
    vmadl voutb, vbb, vw_center
    vmadl voutb, vab, vw_side
    
    # round green blue
    vadd voutg, vg_round
    vadd voutb, vb_round
    
    # mask result
    vand voutr, voutr, vr_mask
    vand voutg, voutg, vg_mask
    vand voutb, voutb, vb_mask
    
    # load next row
    lqv vcr, 0, 64, buffer_ptr
    
    # recombine channels
    vor voutr, voutr, voutg
    vor voutr, voutr, voutb

    # mask next
    vand vcb, vcr, vb_mask
    vand vcg, vcr, vg_mask
    vand vcr, vcr, vr_mask
    
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
    jal DMAIn
    li dmem_address, %lo(SCREEN_BUFFER)

    li buffer_ptr, %lo(SCREEN_BUFFER)
    addi buffer_ptr_end, buffer_ptr, 2 * 8 * 117

    j BlurCmd_Blur_BatchLoopPoint
    addi pos_y, 1
BlurCmd_Blur_ColEnd:    

    # sqv $v00, 0, 0, buffer_ptr
    # sqv $v00, 0, 16, buffer_ptr
    # sqv $v00, 0, 32, buffer_ptr
    
    # li dma_size, DMA_SIZE(16, 120)
    # li row_stride, 640
    # move dram_address, a1
    # li dmem_address, %lo(SCREEN_BUFFER)
    nop

    j RSPQ_Loop
    nop

