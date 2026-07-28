
#include <rsp_queue.inc>

#define SIM_BUFFER_SIZE 1024

#define SPRING_CONSTANT     0x4999
#define DAMPING_CONSTANT    0xFFF0

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_ProcessBlock, 16
        RSPQ_DefineCommand WaterCmd_Profile, 4
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4
        DEBUG:
            .half 0x0000, 0x0000, 0x0000, 0x0000
        TIMER:
            .word 0
        .align 4
        SPRING_K: 
            .half 0x0000, SPRING_CONSTANT, SPRING_CONSTANT, SPRING_CONSTANT
            .half SPRING_CONSTANT, SPRING_CONSTANT, SPRING_CONSTANT, 0x0000
        .align 4
        VEL_DAMPING:
            .half 0xFFFF, DAMPING_CONSTANT, DAMPING_CONSTANT, DAMPING_CONSTANT
            .half DAMPING_CONSTANT, DAMPING_CONSTANT, DAMPING_CONSTANT, 0xFFFF
        .align 4
        DELTA_TIME: 
            .half 0x0000, 0x0888, 0x0888, 0x0888
            .half 0x0888, 0x0888, 0x0888, 0x0000
    RSPQ_EndSavedState
.bss
    .align 4
    VEL:     .space SIM_BUFFER_SIZE
    IN_POS:  .space SIM_BUFFER_SIZE
    OUT_POS: .space SIM_BUFFER_SIZE

#define VECTOR_SIZE     16

#define FRAME_STEP      6
#define VEL_FRAME_STEP  12

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
#define ptr         t6
#define vel_stride  t7

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

    andi row_stride, a0, 0xFFF
    # same as
    # (((row_count + 2) - 1) << 12) | (row_stride -  1)
    # the +2 is to add padding, the -1 is to properly fill the dma register
    li dma_size, 0xFFFFFF
    and t6, dma_size, a0
    addi dma_size, t6, (1 << 12) - 1

    # double dma size for velocity
    add dma_size, dma_size, row_stride
    sll vel_stride, row_stride, 1
    sll row_stride, row_stride, 1

    # load the velocity block
    move dram_address, vel_dmem
    jal DMAIn
    li dmem_address, %lo(VEL)

    # put back sizes for position copy
    srl row_stride, row_stride, 1
    addi dma_size, t6, (1 << 12) - 1
    
    # Load the position block
    move dram_address, pos_dmem
    jal DMAIn
    li dmem_address, %lo(IN_POS)

    li ptr, %lo(SPRING_K)
    lqv spring_k, 0x00, ptr
    lqv vel_damping, 0x10, ptr
    lqv dt, 0x20, ptr

    li row_pos_ptr, %lo(IN_POS)
    li row_vel_ptr, %lo(VEL)
    li row_out_ptr, %lo(OUT_POS)

    add row_pos_ptr, row_pos_ptr, row_stride
    add row_vel_ptr, row_vel_ptr, vel_stride
    add row_out_ptr, row_out_ptr, row_stride
WaterCmd_ProcessBlock_NextRow:
    add pos_ptr, row_pos_ptr, zero
    sub up_ptr, row_pos_ptr, row_stride
    add down_ptr, row_pos_ptr, row_stride
    addi left_ptr, row_pos_ptr, -1
    addi right_ptr, row_pos_ptr, 1
    add vel_ptr, row_vel_ptr, zero
    add out_ptr, row_out_ptr, zero

    # load the first row without prev out position
    lpv pos_center, 0, pos_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_left, 0, left_ptr
    lpv pos_right, 0, right_ptr
    lqv vel, 0, vel_ptr
    lrv vel, VECTOR_SIZE, vel_ptr

    # -2 for 1 pixel of padding on each side
    addi col_count, row_stride, -FRAME_STEP - 2

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

    # round position
    vadd pos_center, pos_center, K128

    # apply pos by delta time (masked)
    vmudm vel, vel, dt
    vadd pos_center, pos_center, vel

    # save position
    spv pos_center, 0, out_ptr

    # increment pointers
    addi pos_ptr, pos_ptr, FRAME_STEP
    addi left_ptr, left_ptr, FRAME_STEP
    addi right_ptr, right_ptr, FRAME_STEP
    addi up_ptr, up_ptr, FRAME_STEP
    addi down_ptr, down_ptr, FRAME_STEP
    addi vel_ptr, vel_ptr, VEL_FRAME_STEP
    addi out_ptr, out_ptr, FRAME_STEP
    addi col_count, col_count, -FRAME_STEP
    
    # check if row has ended
    bltz col_count, WaterCmd_ProcessBlock_IncRow
    nop

    ####################
    # load next
    lpv pos_center, 0, pos_ptr
    # preserve pixel on edge, this is why the load next
    # block must be different than the one above
    lbv pos_center.e0, 0, out_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_left, 0, left_ptr
    lpv pos_right, 0, right_ptr
    lqv vel, 0, vel_ptr
    lrv vel, VECTOR_SIZE, vel_ptr

    j WaterCmd_ProcessBlock_NextFrame
    nop

WaterCmd_ProcessBlock_IncRow:

    # check if rows have finished
    addi row_count, row_count, -1
    blez row_count, WaterCmd_ProcessBlock_Exit

    # increment pointers
    add row_pos_ptr, row_pos_ptr, row_stride
    add row_vel_ptr, row_vel_ptr, vel_stride
    j WaterCmd_ProcessBlock_NextRow
    add row_out_ptr, row_out_ptr, row_stride

    # end loop
WaterCmd_ProcessBlock_Exit:
    # dont output the top and bottom padding rows
    addi dma_size, dma_size, -(2 << 12)
    
    # Save the position block
    add dram_address, out_dmem, row_stride
    jal DMAOut
    addi dmem_address, row_stride, %lo(OUT_POS)

    # double size for velocity block transfer
    add dma_size, dma_size, row_stride
    sll row_stride, row_stride, 1

    # Save the velocity block
    add dram_address, vel_dmem, row_stride
    addi dmem_address, row_stride, %lo(VEL)
    jal_and_j DMAOut, RSPQ_Loop

    ##########################################
    # WaterCmd_Profile
    # 
    # a0: is start
WaterCmd_Profile:
    andi a0, a0, 0x1
    bne a0, zero, WaterCmd_Profile_SkipDelta
    mfc0 t0, COP0_DP_CLOCK

    lw t1, %lo(TIMER)
    sub t0, t0, t1
WaterCmd_Profile_SkipDelta:
    j RSPQ_Loop
    sw t0, %lo(TIMER)

    ##########################################
    # WaterCmd_Apply
    # a0: index_range_count
    # a1: pos_buffer
    # a2: vtx_buffer
    # a3: index_range_buffer
WaterCmd_Apply:
    j RSPQ_Loop
    nop