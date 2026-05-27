
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_ProcessBlock, 16
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4
        ADJACENT_WEIGHT: 
            .half 0x0000, 0x0080, 0x0080, 0x0080
            .half 0x0080, 0x0080, 0x0080, 0x0000
        .align 4
        CENTER_WEIGHT: 
            .half 0x0000, 0x0800, 0x0800, 0x0800
            .half 0x0800, 0x0800, 0x0800, 0x0000
        .align 4
        DELTA_TIME: 
            .half 0x0000, 0x0888, 0x0888, 0x0888
            .half 0x0888, 0x0888, 0x0888, 0x0000
    RSPQ_EndSavedState
.bss
    .align 4
    VEL:     .space 128
    IN_POS:  .space 128
    OUT_POS: .space 128
.text
    ##########################################
    # WaterCmd_ProcessBlock
    # 
    # a0: stride
    # a1: vel
    # a2: in_pos
    # a3: out_pos
WaterCmd_ProcessBlock:
    # Load the velocity block
    move t1, a0
    move s0, a1
    li s4, %lo(VEL)
    jal DMAIn
    li t0, DMA_SIZE(16, 8)
    
    # Load the position block
    move t1, a0
    move s0, a2
    li s4, %lo(IN_POS)
    jal DMAIn
    li t0, DMA_SIZE(16, 8)

    #define pos_ptr s1
    #define left_ptr s2
    #define right_ptr s3
    #define vel_ptr s5
    #define out_ptr s4


    #define adj_weight $v01
    #define center_weight $v02
    #define dt $v03
    
    #define pos_neg $v04
    #define pos_right $v05
    #define pos_left $v06

    #define pos_up $v07
    #define pos_down $v08

    #define pos_center $v09
    #define pos_offset $v10
    #define vel $v11

    li t0, %lo(ADJACENT_WEIGHT)
    lqv adj_weight, 0x00, t0
    lqv center_weight, 0x10, t0
    lqv dt, 0x20, t0

    li pos_ptr, %lo(IN_POS)
    li left_ptr, %lo(IN_POS)+14
    li right_ptr, %lo(IN_POS)+2
    li vel_ptr, %lo(VEL)
    li out_ptr, %lo(OUT_POS)
WaterCmd_ProcessBlock_Next:
    # load data
    lqv pos_center, 16, pos_ptr
    lqv pos_up, 0, pos_ptr
    lqv pos_down, 32, pos_ptr
    lrv pos_left, 16, left_ptr
    lqv pos_right, 16, right_ptr
    lqv vel, 16, vel_ptr
    # accumlate offset * mask factor
    vsub pos_neg, vzero, pos_center
    vmudm pos_offset, pos_up, adj_weight
    vmadm pos_offset, pos_down, adj_weight
    vmadm pos_offset, pos_left, adj_weight
    vmadm pos_offset, pos_right, adj_weight
    vmadm pos_offset, pos_neg, center_weight

    # apply acceleration to velocity
    vadd vel, vel, pos_offset
    sqv vel, 16, vel_ptr

    # apply pos by delta time (masked)
    vmudm vel, vel, dt
    vadd pos_center, pos_center, vel
    sqv pos_center, 16, out_ptr

    # increment pointers
    addi pos_ptr, pos_ptr, 16
    addi left_ptr, left_ptr, 16
    addi right_ptr, right_ptr, 16
    addi vel_ptr, vel_ptr, 16

    li t0, %lo(IN_POS)+128
    sub t0, pos_ptr, t0
    bltz t0, WaterCmd_ProcessBlock_Next
    addi out_ptr, out_ptr, 16

    # Save the position block
    move t1, a0
    move s0, a1
    li s4, %lo(OUT_POS)+16
    li t0, DMA_SIZE(16, 6)
    jal_and_j DMAOut, RSPQ_Loop

