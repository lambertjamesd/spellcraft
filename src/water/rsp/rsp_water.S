
#include <rsp_queue.inc>

#define SIM_BUFFER_SIZE 576

#define SPRING_CONSTANT     0x4999
#define DAMPING_CONSTANT    0xFFF0

#define CMD_OFFSET(offset, cmdsize) (%lo(RSPQ_DMEM_BUFFER) + (offset) - (cmdsize))

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_ProcessBlock, 16
        RSPQ_DefineCommand WaterCmd_Profile, 4
        RSPQ_DefineCommand WaterCmd_Apply, 40
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
        .align 4
        CONSTANTS:
            .half SPRING_CONSTANT, DAMPING_CONSTANT, 0x0888, 0
            .half 0, 0, 0, 0
    RSPQ_EndSavedState
.bss
    .align 4
    VEL:     .space SIM_BUFFER_SIZE*2
    IN_POS:  .space SIM_BUFFER_SIZE
    OUT_POS: .space SIM_BUFFER_SIZE

#define VECTOR_SIZE     16

#define FRAME_STEP      6
#define VEL_FRAME_STEP  12

#define VECTOR_SIZE_LOG2        4
#define VECTOR_SIZE             16

#define MAX_INDEX_BATCH_COUNT   32
#define INDEX_RANGE_SIZE_LOG2   2
#define INDEX_RANGE_SIZE        (1 << INDEX_RANGE_SIZE_LOG2)
#define MAX_INDEX_BATCH_SIZE    (MAX_INDEX_BATCH_COUNT*INDEX_RANGE_SIZE)

#define MAX_VTX_SIZE            ((SIM_BUFFER_SIZE*2)-MAX_INDEX_BATCH_SIZE)
#define MAX_VTX_COUNT           (MAX_VTX_SIZE / VECTOR_SIZE)

#define INDEX_BUFFERS   VEL
#define VTX_BUFFER      (VEL+MAX_INDEX_BATCH_SIZE)

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

#define constants       $v01
#define c_spring_k      $v01.e0
#define c_vel_damping   $v01.e1
#define c_dt            $v01.e2

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

.macro water_sim_chunk
    # accumlate offset * mask factor
    vmadm pos_offset, pos_left, K16384
    vmadm pos_offset, pos_right, K16384
    vmudm pos_offset, pos_up, K16384
    vmadm pos_offset, pos_down, K16384
    vsub pos_offset, pos_offset, pos_center
    vmudm vel, vel, c_vel_damping
    # apply acceleration to velocity
    vmadm vel, pos_offset, c_spring_k

    # save velocity
    sqv vel, 0, vel_ptr
    srv vel, VECTOR_SIZE, vel_ptr

    # round position
    vadd pos_center, pos_center, K128

    # apply pos by delta time
    vmudm vel, vel, c_dt
    vadd pos_center, pos_center, vel

    # save position
    spv pos_center, 0, out_ptr
.endm

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
    # a0: index_range_size
    # a1: pos_buffer
    # a2: vtx_buffer
    # a3: index_range_buffer
    # CMD_ADDR(16, ...) min.xy
    # CMD_ADDR(20, ...) size_inv.xz
WaterCmd_Apply:
    .set noat
    #define vtx_pos         $v01
    #define min_pos         $v02
    #define size_inv        $v03
    #define sim_size        $v04
    #define vtmp            $v05
    #define voffset         $v06

    #define vtx_ptr         t2
    #define max_vtx_ptr     t3
    #define clip_mask       t4
    #define index_ptr       t5
    #define max_index_ptr   t6
    #define idx_range_ptr   t7
    #define idx_range_end   t8

    #define index_range_size   a0
    #define pos_buffer         a1
    #define vtx_buffer         a2
    #define index_range_buffer a3
    

    li dma_size, 32*32-1
    move row_stride, zero
    move dram_address, pos_buffer
    jal DMAIn
    li dmem_address, %lo(IN_POS)

    addi t4, rspq_dmem_buf_ptr, CMD_OFFSET(16, 24)
    llv min_pos 0, 0, t4
    llv min_pos 8, 0, t4
    
    llv size_inv 0, 4, t4

    li t4, 32
    mtc2 t4, sim_size.e0

    andi index_range_size, a0, 0xFFFF
    sll index_range_size, index_range_size, INDEX_RANGE_SIZE_LOG2

WaterCmd_NextIndexBatch:
    beq zero, index_range_size, RSPQ_Loop

    addi $at, index_range_size, -MAX_INDEX_BATCH_SIZE
    bgtz $at, WaterCmd_LoadIndexBatch
    li dma_size, MAX_INDEX_BATCH_SIZE

    # load index buffers
    move dma_size, index_range_size
WaterCmd_LoadIndexBatch:
    li idx_range_ptr, %lo(INDEX_BUFFERS)
    add idx_range_end, idx_range_ptr, dma_size

    sub index_range_size, index_range_size, dma_size
    move dram_address, index_range_buffer
    add index_range_buffer, index_range_buffer, dma_size
    addi dma_size, dma_size, -1
    move row_stride, zero
    jal DMAIn
    li dmem_address, %lo(INDEX_BUFFERS)
WaterCmd_NextIndexRange:
    beq idx_range_ptr, idx_range_end, WaterCmd_NextIndexBatch
    lhu vtx_ptr, 0(idx_range_ptr)
    lhu max_vtx_ptr, 2(idx_range_ptr)

    sub dma_size, max_vtx_ptr, vtx_ptr
    addi $at, dma_size, -MAX_VTX_SIZE
    blez $at, WaterCmd_IncRangePtr
    add dram_address, vtx_ptr, vtx_buffer

    li dma_size, MAX_VTX_SIZE
    add vtx_ptr, vtx_ptr, dma_size
    j WaterCmd_LoadVtxRange
    sh vtx_ptr, 0(idx_range_ptr)
WaterCmd_IncRangePtr:
    addi idx_range_ptr, idx_range_ptr, INDEX_RANGE_SIZE
WaterCmd_LoadVtxRange:
    li vtx_ptr, %lo(VTX_BUFFER)
    add max_vtx_ptr, vtx_ptr, dma_size

    sll dma_size, dma_size, 12 - INDEX_RANGE_SIZE_LOG2
    addi dma_size, dma_size, -(1 << 12) + (INDEX_RANGE_SIZE - 1)
    li row_stride, 16
    jal DMAIn
    li dmem_address, %lo(VTX_BUFFER)

WaterCmd_NextVtx:
    beq vtx_ptr, max_vtx_ptr, WaterCmd_WriteBackVtx

    lqv vtx_pos, 0, 0, vtx_ptr
    vsub vtx_pos, vtx_pos, min_pos

    vmudm vtx_pos, vtx_pos, size_inv.e0

    #define clip_mask_read  t4
    vlt vtmp, vtx_pos, vzero
    cfc2 clip_mask_read, COP2_CTRL_VCC

    vge vtmp, vtx_pos, sim_size.e0
    cfc2 clip_mask, COP2_CTRL_VCC

    or clip_mask, clip_mask, clip_mask_read
    #undef clip_mask_read
    #define pos_read    t4
    #define pixel_index t5

    andi $at, clip_mask, 0x03
    bne $at, zero, WaterCmd_SkipA

    mfc2 pos_read, vtx_pos.e1
    sll pixel_index, pos_read, 5
    mfc2 pos_read, vtx_pos.e0
    add pixel_index, pixel_index, pos_read

    lh pos_read, %lo(IN_POS)(pixel_index)

    mtc2 pos_read, voffset.e2

WaterCmd_SkipA:
    andi $at, clip_mask, 0x30
    bne $at, zero, WaterCmd_SkipB

    mfc2 pos_read, vtx_pos.e5
    sll pixel_index, pos_read, 5
    mfc2 pos_read, vtx_pos.e4
    add pixel_index, pixel_index, pos_read

    lh pos_read, %lo(IN_POS)(pixel_index)

    mtc2 pos_read, voffset.e6
WaterCmd_SkipB:
    #undef pos_read

    # scale the offset
    vmudm voffset, voffset, size_inv.e1
    # add original pos to offset
    vadd voffset, voffset, vtx_pos.h3

    # store new vertical position
    ssv voffset, 4, 4, vtx_ptr
    ssv voffset, 12, 12, vtx_ptr

    j WaterCmd_NextVtx
    addi vtx_ptr, vtx_ptr, 16

WaterCmd_WriteBackVtx:
    jal_and_j DMAOut, WaterCmd_NextIndexRange

    
    ##########################################
    # WaterCmd_ProcessBlock
    # 
    # a0: h:12 w:12
    # a1: vel
    # a2: in_pos
    # a3: out_pos
WaterCmd_ProcessBlockV2:
    .set at
    srl row_count, a0, Y_STRIDE_OFFSET
    andi row_count, 0xFF
    
    andi row_stride, a0, 0xFFF
    # same as
    # (((row_count + 2) - 1) << 12) | (row_stride -  1)
    # the +2 is to add padding, the -1 is to properly fill the dma register
    li dma_size, 0xFFFFFF
    and dma_size, a0    
    addi dma_size, -(1 << 12) - 1

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
    addi dma_size, (2 << 12)
    sub dma_size, row_stride
    
    # Load the position block
    move dram_address, pos_dmem
    jal DMAIn
    li dmem_address, %lo(IN_POS)

    # load constants
    li ptr, %lo(CONSTANTS)
    lqv constants, 0x00, ptr

    li row_pos_ptr, %lo(IN_POS)
    li row_vel_ptr, %lo(VEL)
    li row_out_ptr, %lo(OUT_POS)

    add row_pos_ptr, row_pos_ptr, row_stride
    add row_vel_ptr, row_vel_ptr, vel_stride
    add row_out_ptr, row_out_ptr, row_stride
WaterCmd_ProcessBlockV2_NextRow:
    add pos_ptr, row_pos_ptr, zero
    sub up_ptr, row_pos_ptr, row_stride
    add down_ptr, row_pos_ptr, row_stride
    addi left_ptr, row_pos_ptr, -1
    addi right_ptr, row_pos_ptr, 1
    add vel_ptr, row_vel_ptr, zero
    add out_ptr, row_out_ptr, zero
    

    srl col_count, a0, 3
    andi col_count, 0x1F

WaterCmd_ProcessBlockV2_RowStart:
    lpv pos_left, 0, left_ptr
    # pad left with 0
    mtc2 zero, pos_left.e0
    lpv pos_right, 0, right_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_center, 0, pos_ptr

    lqv vel, 0, vel_ptr

    ble col_count, 1, WaterCmd_ProcessBlockV2_LastChunk
    nop
WaterCmd_ProcessBlockV2_RowLoop:
    water_sim_chunk;

    addi left_ptr, 8
    addi right_ptr, 8
    addi up_ptr, 8
    addi down_ptr, 8
    addi pos_ptr, 8
    addi out_ptr, 8
    addi vel, 16
    
    lpv pos_left, 0, left_ptr
    lpv pos_right, 0, right_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_center, 0, pos_ptr

    lqv vel, 0, vel_ptr

    bgt col_count, 1, WaterCmd_ProcessBlockV2_RowLoop
    addi col_count, -1

WaterCmd_ProcessBlockV2_LastChunk:
    # pad right with 0
    mtc2 zero, pos_right.e7
    water_sim_chunk;

    ble row_count, 1, WaterCmd_ProcessBlockV2_WriteResult
    addi row_count, -1

    add row_pos_ptr, row_stride
    add row_vel_ptr, vel_stride
    j WaterCmd_ProcessBlockV2_NextRow
    add row_out_ptr, row_stride

WaterCmd_ProcessBlockV2_WriteResult:
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
    move dram_address, vel_dmem
    li dmem_address, %lo(VEL)
    jal_and_j DMAOut, RSPQ_Loop