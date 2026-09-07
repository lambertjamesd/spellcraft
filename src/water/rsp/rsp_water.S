
#include <rsp_queue.inc>

#define SIM_BUFFER_SIZE 576

#define SPRING_CONSTANT     0xFFFF
#define DAMPING_CONSTANT    0xFFFF
#define DT                  0x0888

#define CMD_OFFSET(offset, cmdsize) (%lo(RSPQ_DMEM_BUFFER) + (offset) - (cmdsize))

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_ProcessBlock, 16
        RSPQ_DefineCommand WaterCmd_Profile, 4
        RSPQ_DefineCommand WaterCmd_Apply, 20
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4    
        DEBUG:
            .half 0x0000, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
        TIMER:
            .word 0
        .align 4
        CONSTANTS:
            .half SPRING_CONSTANT, DAMPING_CONSTANT, DT, 0
            .half 0, 0, 0, 0
    RSPQ_EndSavedState
.bss
    .align 4
    VEL:     .space SIM_BUFFER_SIZE*2
    IN_POS:  .space SIM_BUFFER_SIZE
    OUT_POS: .space SIM_BUFFER_SIZE

#define VECTOR_SIZE     16

#define VECTOR_SIZE_LOG2        4
#define VECTOR_SIZE             16

#define MAX_VTX_SIZE            (SIM_BUFFER_SIZE*2)
#define MAX_VTX_COUNT           (MAX_VTX_SIZE / VECTOR_SIZE)

#define VTX_BUFFER      VEL

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
#define pos_stride  t8

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
    vmudm pos_offset, pos_left, K16384
    vmadm pos_offset, pos_right, K16384
    vmadm pos_offset, pos_up, K16384
    vmadm pos_offset, pos_down, K16384
    vsub pos_offset, pos_center
    vmudm vel, vel, c_vel_damping
    # apply acceleration to velocity
    vmadm vel, pos_offset, c_spring_k

    # save velocity
    sqv vel, 0, vel_ptr

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
    # a0: h:12 w:12
    # a1: vel
    # a2: in_pos
    # a3: out_pos
WaterCmd_ProcessBlock:
    .set at
    srl row_count, a0, Y_STRIDE_OFFSET
    andi row_count, 0xFF
    
    andi pos_stride, a0, 0xFFF
    # same as
    # (((row_count + 2) - 1) << 12) | (pos_stride -  1)
    # the +2 is to add padding, the -1 is to properly fill the dma register
    li dma_size, 0xFFFFFF
    and dma_size, a0    
    addi dma_size, -(1 << 12) - 1

    # double dma size for velocity
    add dma_size, dma_size, pos_stride
    sll vel_stride, pos_stride, 1
    li row_stride, 64

    # load the velocity block
    move dram_address, vel_dmem
    jal DMAIn
    li dmem_address, %lo(VEL)

    # put back sizes for position copy
    li row_stride, 32
    # +2 height
    addi dma_size, (2 << 12)
    # u16 to u8
    sub dma_size, pos_stride
    
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

    add row_pos_ptr, row_pos_ptr, pos_stride
WaterCmd_ProcessBlock_NextRow:
    add pos_ptr, row_pos_ptr, zero
    sub up_ptr, row_pos_ptr, pos_stride
    add down_ptr, row_pos_ptr, pos_stride
    addi left_ptr, row_pos_ptr, -1
    addi right_ptr, row_pos_ptr, 1
    add vel_ptr, row_vel_ptr, zero
    add out_ptr, row_out_ptr, zero
    
    srl col_count, a0, 3
    andi col_count, 0x1F

WaterCmd_ProcessBlock_RowStart:
    lpv pos_left, 0, left_ptr
    # pad left with 0
    mtc2 zero, pos_left.e0
    lpv pos_right, 0, right_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_center, 0, pos_ptr

    lqv vel, 0, vel_ptr

    ble col_count, 1, WaterCmd_ProcessBlock_LastChunk
    nop
WaterCmd_ProcessBlock_RowLoop:
    water_sim_chunk;

    addi left_ptr, 8
    addi right_ptr, 8
    addi up_ptr, 8
    addi down_ptr, 8
    addi pos_ptr, 8
    addi out_ptr, 8
    addi vel_ptr, 16
    
    lpv pos_left, 0, left_ptr
    lpv pos_right, 0, right_ptr
    lpv pos_up, 0, up_ptr
    lpv pos_down, 0, down_ptr
    lpv pos_center, 0, pos_ptr

    lqv vel, 0, vel_ptr

    bgt col_count, 1, WaterCmd_ProcessBlock_RowLoop
    addi col_count, -1

WaterCmd_ProcessBlock_LastChunk:
    # pad right with 0
    mtc2 zero, pos_right.e7
    water_sim_chunk;

    ble row_count, 1, WaterCmd_ProcessBlock_WriteResult
    addi row_count, -1

    add row_pos_ptr, pos_stride
    add row_vel_ptr, vel_stride
    j WaterCmd_ProcessBlock_NextRow
    add row_out_ptr, pos_stride

WaterCmd_ProcessBlock_WriteResult:
    # dont output the top and bottom padding rows
    addi dma_size, dma_size, -(2 << 12)
    
    # Save the position block
    move dram_address, out_dmem
    jal DMAOut
    li dmem_address, %lo(OUT_POS)

    # double size for velocity block transfer
    add dma_size, dma_size, pos_stride
    li row_stride, 64

    # Save the velocity block
    move dram_address, vel_dmem
    li dmem_address, %lo(VEL)
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
    # a0: vertex_count
    # a1: pos_buffer
    # a2: vtx_buffer
    # a3: min.xy
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

    #define vertex_count       a0
    #define pos_buffer         a1
    #define vtx_buffer         a2
    #define min.xy             a3

    li dma_size, 32*32-1
    move row_stride, zero
    move dram_address, pos_buffer
    jal DMAIn
    li dmem_address, %lo(IN_POS)

    li t4, %lo(VEL)
    sw a3, %lo(VEL)
    lsv min_pos 0, 0, t4
    lsv min_pos 4, 2, t4
    lsv min_pos 8, 0, t4
    lsv min_pos 12, 0, t4

    addi t4, rspq_dmem_buf_ptr, CMD_OFFSET(16, 20)
    
    llv size_inv 0, 0, t4

    li t4, 32
    mtc2 t4, sim_size.e0

    andi vertex_count, 0xFFFF
    beq zero, vertex_count, RSPQ_Loop

    # load vertices
    sll dma_size, vertex_count, 12
    addi dma_size, dma_size, -(1 << 12) + 15
    li row_stride, 32
    move dram_address, vtx_buffer
    jal DMAIn
    li dmem_address, %lo(VTX_BUFFER)
    
    li vtx_ptr, %lo(VTX_BUFFER)
    sll max_vtx_ptr, vertex_count, 4
    add max_vtx_ptr, vtx_ptr

WaterCmd_NextVtx:
    beq vtx_ptr, max_vtx_ptr, WaterCmd_WriteBackVtx

    lqv vtx_pos, 0, 0, vtx_ptr
    vsub vtx_pos, vtx_pos, min_pos

    vxor voffset, voffset

    vmudm vtx_pos, vtx_pos, size_inv.e0

    #define clip_mask_read  t5
    vlt vtmp, vtx_pos, vzero
    cfc2 clip_mask_read, COP2_CTRL_VCC

    vge vtmp, vtx_pos, sim_size.e0
    cfc2 clip_mask, COP2_CTRL_VCC

    or clip_mask, clip_mask_read
    #undef clip_mask_read
    #define pos_read    t5
    #define pixel_index t6

    andi $at, clip_mask, 0x05
    bne $at, zero, WaterCmd_SkipA

    mfc2 pos_read, vtx_pos.e2
    sll pixel_index, pos_read, 5
    mfc2 pos_read, vtx_pos.e0
    add pixel_index, pos_read

    lb pos_read, %lo(IN_POS)(pixel_index)
    sll pos_read, 8

    mtc2 pos_read, voffset.e1

WaterCmd_SkipA:
    andi $at, clip_mask, 0x50
    bne $at, zero, WaterCmd_SkipB

    mfc2 pos_read, vtx_pos.e6
    sll pixel_index, pos_read, 5
    mfc2 pos_read, vtx_pos.e4
    add pixel_index, pixel_index, pos_read

    lb pos_read, %lo(IN_POS)(pixel_index)
    sll pos_read, 8

    mtc2 pos_read, voffset.e5
WaterCmd_SkipB:
    #undef pos_read

    # scale the offset
    vmudm voffset, voffset, size_inv.e1

    # store new vertical position
    ssv voffset, 2, 2, vtx_ptr
    ssv voffset, 10, 10, vtx_ptr

    j WaterCmd_NextVtx
    addi vtx_ptr, vtx_ptr, 16

WaterCmd_WriteBackVtx:
    jal_and_j DMAOut, RSPQ_Loop
