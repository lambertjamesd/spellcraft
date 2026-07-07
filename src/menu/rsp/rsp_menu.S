
#include <rsp_queue.inc>
#include "rsp_menu.inc"

.data

# fmt xxyy uuww rgba 0000

#define VERTEX_SIZE     16

    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MenuCmd_MoveTo, 16
        RSPQ_DefineCommand MenuCmd_LineTo, 16
        RSPQ_DefineCommand MenuCmd_Mtx, 8
        RSPQ_DefineCommand MenuCmd_MtxPop, 4
        RSPQ_DefineCommand MenuCmd_SetStack, 8
    RSPQ_EndOverlayHeader
    RSPQ_BeginSavedState
        .align 4
        TEST: 
            .half 0, 0, 0, 0, 0, 0, 0, 0
            .half 0, 0, 0, 0, 0, 0, 0, 0
        .align 4
        MTX_TOP: 
            .half 0x0001, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0001, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
        .align 3
        MTX_STACK_PTR:  .word 0
        MTX_STACK_SIZE: .byte 0
        MTX_STACK_MAX: .byte 10

        .align 2
        VIEWPORT: .half 0, 320, 0, 240
        .align 4
        VERTEX_POINTS: .space VERTEX_SIZE*2
    RSPQ_EndSavedState

.bss
    .align 4
    VERTEX_INPUT: .space VERTEX_SIZE
    .align 4
    MTX_INPUT: .space 32

#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

.macro vmifxif oi, of, ai, af, bi, bf
    vmudl \of, \af, \bf
    vmadm \of, \ai, \bf
    vmadn \of, \af, \bi
    vmadh \oi, \ai, \bi
.endm

.macro vmifxi oi, of, ai, af, bi
    vmudn \of, \af, \bi
    vmadh \oi, \ai, \bi
.endm

.macro vaddif oi, of, ai, af, bi, bf
    vaddc \of, \af, \bf
    vadd \oi, \ai, \bi
.endm

.text
    .set noat

    ##########################################
    # MenuCmd_MoveTo
    # 
    # a0: unused
    # a1: xxyy
    # a2: uuww
    # a3: rgba
MenuCmd_MoveTo:
    jal MulPoint
    sw a1, %lo(VERTEX_INPUT)

    lw t2, %lo(VERTEX_INPUT)
    sw t2, %lo(TEST)

    j RSPQ_Loop
    nop

    ##########################################
    # MenuCmd_LineTo
    # 
    # a0: unused
    # a1: xxyy
    # a2: uuww
    # a3: rgba
MenuCmd_LineTo:
    j RSPQ_Loop
    nop

    ##########################################
    # MenuCmd_Mtx
    # a0 MENU_MTX_MUL | MENU_MTX_PUSH
    # a1 mtx_addr
MenuCmd_Mtx:
    andi $at, a0, MENU_MTX_PUSH
    beq $at, zero, MenuCmd_Mtx_SkipPush

    lb t2, %lo(MTX_STACK_SIZE)
    lb t3, %lo(MTX_STACK_MAX)
    beq t2, t3, RSPQ_Loop
    addi t2, t2, 1
    sb t2, %lo(MTX_STACK_SIZE)

    li dmem_address, %lo(MTX_TOP)
    lw dram_address, %lo(MTX_STACK_PTR)
    li dma_size, DMA_SIZE(32, 1)
    jal DMAOut
    li row_stride, 0

    addi dram_address, dram_address, 4
    sw dram_address, %lo(MTX_STACK_PTR)
MenuCmd_Mtx_SkipPush:
    andi $at, a0, MENU_MTX_MUL
    beq $at, zero, MenuCmd_Mtx_SkipMul
    
    li dmem_address, %lo(MTX_INPUT)
    addi dram_address, a1, 0
    li dma_size, DMA_SIZE(32, 1)
    jal DMAIn
    li row_stride, 0

    jal_and_j MulMtx, RSPQ_Loop

MenuCmd_Mtx_SkipMul:
    li dmem_address, %lo(MTX_TOP)
    addi dram_address, a1, 0
    li dma_size, DMA_SIZE(32, 1)
    li row_stride, 0
    jal_and_j DMAIn, RSPQ_Loop

    ##########################################
    # MenuCmd_Pop
    # a0 count
MenuCmd_MtxPop:
    andi a0, a0, 0xFF
    lb t2, %lo(MTX_STACK_SIZE)
    sub t3, t2, a0

    # prevent underflow
    bgez t3, MenuCmd_MtxPopDoPop
    nop
    addi a0, t2, 0
    xor t3, t3, t3
MenuCmd_MtxPopDoPop:
    # check if count is zero, do nothing
    beq a0, zero, RSPQ_Loop
    sll a0, a0, 2

    sb t3, %lo(MTX_STACK_SIZE)
    lw dram_address, %lo(MTX_STACK_PTR)
    sub dram_address, dram_address, a0
    sw dram_address, %lo(MTX_STACK_PTR)

    # load prev matrix
    li dmem_address, %lo(MTX_TOP)
    li dma_size, DMA_SIZE(32, 1)
    li row_stride, 0
    jal_and_j DMAIn, RSPQ_Loop

    
    ##########################################
    # MenuCmd_Line
    # a0 max size
    # a1 mtx_addr
MenuCmd_SetStack:
    li $at, 57
    sb $at, %lo(TEST)

    sb a0, %lo(MTX_STACK_MAX)
    sw a1, %lo(MTX_STACK_PTR)
    j RSPQ_Loop
    sb zero, %lo(MTX_STACK_SIZE)
    
    ##########################################
    # MenuCmd_Line
    # a0 a
    # a1 b
DrawLine:
    # - if a == b
    #     return

    # convert vertex index to vertex pointers
    sll	t0, t0, 4
    sll t1, t1, 4
    addi t0, t0, %lo(VERTEX_POINTS)
    addi t1, t1, %lo(VERTEX_POINTS)

    # load a and b
    llv $v01, 0, 0, t0
    llv $v02, 0, 0, t1

    vsub $v03, $v01, $v02

    vmudm $v04, $v03, $v03 # multiply I*F
    vmadn $v05, $v00, $v00 # store fractional result

    vaddc $v05, $v05, $v05.h1
    vadd $v04, $v04, $v04.h1

    vrcph $v06.e0, $v04.e0
    vrcpl $v07.e0, $v05.e0
    vrcph $v06.e0, $v00.e0
    # a - b

    

    jr ra
    nop

    ##########################################
    # Does the matrix operation MTX_TOP = MTX_TOP * MTX_INPUT

    #define mtx_topi         $v01
    #define mtx_topf         $v02

    #define input_vtxi       $v03

    #define input_topi       $v03
    #define input_topf       $v04
    #define input_bottomi    $v05
    #define input_bottomf    $v06
MulMtx:
    li t2, %lo(MTX_TOP)
    li t3, %lo(MTX_INPUT)
    
    lqv mtx_topi, 0x00, t2
    lqv mtx_topf, 0x10, t2
    ldv input_topi, 0, 0x00, t3
    ldv input_topi, 8, 0x00, t3
    ldv input_topf, 0, 0x010, t3
    ldv input_topf, 8, 0x010, t3
    ldv input_bottomi, 0, 0x08, t3
    ldv input_bottomi, 8, 0x08, t3
    ldv input_bottomf, 0, 0x018, t3
    ldv input_bottomf, 8, 0x018, t3

    # matrix multiply
    vmifxif $v08, $v07, input_topi, input_topf, mtx_topi.h0, mtx_topf.h0
    vmifxif $v10, $v09, input_bottomi, input_bottomf, mtx_topi.h1, mtx_topf.h1
    vaddif $v08, $v07, $v08, $v07, $v10, $v09
    
    # store rotate/scale
    # integer half
    slv $v08, 0, 0x00, t2
    slv $v08, 8, 0x08, t2
    # add translate
    vadd $v08, $v08, mtx_topi.h2
    # fixed point half
    slv $v07, 0, 0x10, t2
    slv $v07, 8, 0x18, t2
    # add translate
    vadd $v07, $v07, mtx_topf.h2

    # store translate
    ssv $v08, 4, 0x04, t2
    ssv $v08, 12, 0x0C, t2

    # store translate
    ssv $v07, 4, 0x14, t2
    jr ra
    ssv $v07, 12, 0x1C, t2

    ##########################################
    # Does the matrix operation VERTEX_INPUT = MTX_TOP * VERTEX_INPUT
MulPoint:
    li t2, %lo(MTX_TOP)
    li t3, %lo(VERTEX_INPUT)
    lqv mtx_topi, 0x00, t2
    lqv mtx_topf, 0x10, t2

    llv input_vtxi, 0, 0, t3
    llv input_vtxi, 8, 0, t3

    vmifxi $v08, $v07, mtx_topi, mtx_topf, input_vtxi

    vaddc $v07, $v07, $v07.h1
    vadd $v08, $v08, $v08.h1

    li t3, %lo(VERTEX_POINTS)

    vaddc $v07, $v07, mtx_topf.h2
    vadd $v08, $v08, mtx_topi.h2

    ssv $v08, 0, 0, t3
    jr ra
    ssv $v08, 8, 2, t3