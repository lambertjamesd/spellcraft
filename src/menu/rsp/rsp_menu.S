
#include <rsp_queue.inc>

.data

# fmt xxyy uuww rgba 0000

#define VERTEX_SIZE     16
#define VERTEX_COUNT    32

    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MenuCmd_Vtx, 8
        RSPQ_DefineCommand MenuCmd_Line, 8
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4
        VTX: .space VERTEX_SIZE*VERTEX_COUNT
    RSPQ_EndSavedState

.bss
    .align 4
    VERTEX_INPUT: .space VERTEX_SIZE*VERTEX_COUNT

#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

.text

    ##########################################
    # MenuCmd_Vtx
    # 
    # a0: size | offset
    # a1: ram_addr
MenuCmd_Vtx:
    andi dmem_address, a0, 0xFFF
    srl dma_size, 12
    addi dram_address, a1, 0
    addi dmem_address, dmem_address, %lo(VERTEX_INPUT)
    andi dma_size, dma_size, 0xFFF
    addi dma_size, dma_size, -1
    jal DMAIn
    li row_stride, 0

    j RSPQ_Loop
    nop

    ##########################################
    # MenuCmd_Line
    # 
    # a0: a0 | b0
    # a1: a1 | b2 | a2 | b2
MenuCmd_Line:
    j RSPQ_Loop
    nop
    
    
    ##########################################
    # MenuCmd_Line
    # t0 a
    # t1 b
DrawLine:
    # - if a == b
    #     return

    # a - b

    jr ra
    nop