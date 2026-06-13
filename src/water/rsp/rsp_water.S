
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_ProcessBlock, 16
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4
        SPRING_K: 
            .half 0x0000, 0x1999, 0x1999, 0x1999
            .half 0x1999, 0x1999, 0x1999, 0x0000
        .align 4
        VEL_DAMPING:
            .half 0xFFFF, 0xFEB8, 0xFEB8, 0xFEB8
            .half 0xFEB8, 0xFEB8, 0xFEB8, 0xFFFF
        .align 4
        DELTA_TIME: 
            .half 0x0000, 0x0888, 0x0888, 0x0888
            .half 0x0888, 0x0888, 0x0888, 0x0000
    RSPQ_EndSavedState
.bss
    .align 4
    VEL:     .space 1024
    IN_POS:  .space 1024
    OUT_POS: .space 1024

#define VECTOR_SIZE     16

#define FRAME_STEP      12

#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

#define up_ptr s0
#define pos_ptr s1
#define down_ptr s2
#define left_ptr s3
#define right_ptr s4
#define out_ptr s5
#define vel_ptr s6
#define row_count s7

#define row_pos_ptr t2
#define row_vel_ptr t3
#define row_out_ptr t4
#define col_count   t5

#define spring_k $v01
#define vel_damping $v02
#define dt $v03

#define pos_neg $v04
#define pos_right $v05
#define pos_left $v06

#define pos_up $v07
#define pos_down $v08

#define pos_center $v09
#define pos_offset $v10
#define vel $v11

#define vel_dmem    a1
#define pos_dmem    a2
#define out_dmem    a3

#define Y_STRIDE_OFFSET 12

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
    srl row_count, a0, Y_STRIDE_OFFSET
    andi row_count, row_count, 0xFF

    andi row_stride, a0, 0xFFFF
    # same as
    # (((row_count + 2) - 1) << 12) | (row_stride -  1)
    # the +2 is to add padding, the -1 is to properly fill the dma register
    addi dma_size, a0, (1 << 12) - 1

    # load the velocity block
    move dram_address, vel_dmem
    jal DMAIn
    li dmem_address, %lo(VEL)
    
    # Load the position block
    move dram_address, pos_dmem
    jal DMAIn
    li dmem_address, %lo(IN_POS)

    li t0, %lo(SPRING_K)
    lqv spring_k, 0x00, t0
    lqv vel_damping, 0x10, t0
    lqv dt, 0x20, t0

    li row_pos_ptr, %lo(IN_POS)
    li row_vel_ptr, %lo(VEL)
    li row_out_ptr, %lo(OUT_POS)

    add row_pos_ptr, row_pos_ptr, row_stride
    add row_vel_ptr, row_vel_ptr, row_stride
    add row_out_ptr, row_out_ptr, row_stride

WaterCmd_ProcessBlock_NextRow:
    add pos_ptr, row_pos_ptr, zero
    sub up_ptr, row_pos_ptr, row_stride
    add down_ptr, row_pos_ptr, row_stride
    addi left_ptr, row_pos_ptr, -2
    addi right_ptr, row_pos_ptr, 2
    add vel_ptr, row_vel_ptr, zero
    add out_ptr, row_out_ptr, zero

    # load the first row without prev out position
    lqv pos_center, 0, pos_ptr
    lrv pos_center, VECTOR_SIZE, pos_ptr
    lqv pos_up, 0, up_ptr
    lrv pos_up, VECTOR_SIZE, up_ptr
    lqv pos_down, 0, down_ptr
    lrv pos_down, VECTOR_SIZE, down_ptr
    lqv pos_left, 0, left_ptr
    lrv pos_left, VECTOR_SIZE, left_ptr
    lqv pos_right, 0, right_ptr
    lrv pos_right, VECTOR_SIZE, right_ptr
    lqv vel, 0, vel_ptr
    lrv vel, VECTOR_SIZE, vel_ptr

    # -4 for 1 pixel of padding on each side
    addi col_count, row_stride, -FRAME_STEP - 4

WaterCmd_ProcessBlock_NextFrame:
    # accumlate offset * mask factor
    vmudm pos_offset, pos_up, K16384
    vmadm pos_offset, pos_down, K16384
    vmadm pos_offset, pos_left, K16384
    vmadm pos_offset, pos_right, K16384
    vsub pos_offset, pos_offset, pos_center
    
    vmudm vel, vel, vel_damping
    # apply acceleration to velocity
    vmadm vel, pos_offset, spring_k

    # save velocity
    sqv vel, 0, vel_ptr
    srv vel, VECTOR_SIZE, vel_ptr

    # apply pos by delta time (masked)
    vmudm vel, vel, dt
    vadd pos_center, pos_center, vel

    # save position
    sqv pos_center, 0, out_ptr
    srv pos_center, VECTOR_SIZE, out_ptr

    # increment pointers
    addi pos_ptr, pos_ptr, FRAME_STEP
    addi left_ptr, left_ptr, FRAME_STEP
    addi right_ptr, right_ptr, FRAME_STEP
    addi vel_ptr, vel_ptr, FRAME_STEP
    addi out_ptr, out_ptr, FRAME_STEP
    addi col_count, col_count, -FRAME_STEP
    
    # check if row has ended
    bltz col_count, WaterCmd_ProcessBlock_IncRow

    ####################
    # load next
    lqv pos_center, 0, pos_ptr
    lrv pos_center, VECTOR_SIZE, pos_ptr
    # preserve pixel on edge, this is why the load next
    # block must be different than the one above
    lsv pos_center.e0, 0, out_ptr
    lqv pos_up, 0, up_ptr
    lrv pos_up, VECTOR_SIZE, up_ptr
    lqv pos_down, 0, down_ptr
    lrv pos_down, VECTOR_SIZE, down_ptr
    lqv pos_left, 0, left_ptr
    lrv pos_left, VECTOR_SIZE, left_ptr
    lqv pos_right, 0, right_ptr
    lrv pos_right, VECTOR_SIZE, right_ptr
    lqv vel, 0, vel_ptr
    lrv vel, VECTOR_SIZE, vel_ptr

    j WaterCmd_ProcessBlock_NextFrame

WaterCmd_ProcessBlock_IncRow:

    # check if rows have finished
    addi row_count, row_count, -1
    blez row_count, WaterCmd_ProcessBlock_Exit

    # increment pointers
    add row_pos_ptr, row_pos_ptr, row_stride
    add row_vel_ptr, row_vel_ptr, row_stride
    j WaterCmd_ProcessBlock_NextRow
    add row_out_ptr, row_out_ptr, row_stride

    # end loop
WaterCmd_ProcessBlock_Exit:
    # dont output the top and bottom padding rows
    addi dma_size, dma_size, -(2 << 12)

    # Save the velocity block
    add dram_address, vel_dmem, row_stride
    jal DMAOut
    addi dmem_address, row_stride, %lo(VEL)

    # Save the position block
    add dram_address, out_dmem, row_stride
    addi dmem_address, row_stride, %lo(OUT_POS)
    jal_and_j DMAOut, RSPQ_Loop

