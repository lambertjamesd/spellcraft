
#include <rsp_queue.inc>
#include "rsp_menu.inc"

.data

# fmt xxyy uuww rgba c000
# xxyy - position                   (0, 2)
# uu - u of uv texture coordinate   (4)
# ww - width of the line            (6)
# rgba - color                      (8)
# c - clipping mask                 (12)

#define VERTEX_SIZE     16

#define VERTEX_RGBA(index)     (%lo(VERTEX_POINTS) + 8 + index * VERTEX_SIZE)
#define VERTEX_U(index)     (%lo(VERTEX_POINTS) + 4 + index * VERTEX_SIZE)
#define VERTEX_WIDTH(index)     (%lo(VERTEX_POINTS) + 6 + index * VERTEX_SIZE)
#define VERTEX_CLIP_MASK(index)     (%lo(VERTEX_POINTS) + 12 + index * VERTEX_SIZE)

#define TRI_VERTEX_SIZE 32
#define TRI_VTX_COUNT   64

#define TRI_VTX(index, offset)     (%lo(RDPQ_TRI_DATA) + offset + index * TRI_VERTEX_SIZE)

    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MenuCmd_MoveTo, 16
        RSPQ_DefineCommand MenuCmd_LineTo, 16
        RSPQ_DefineCommand MenuCmd_Mtx, 8
        RSPQ_DefineCommand MenuCmd_MtxPop, 4
        RSPQ_DefineCommand MenuCmd_SetStack, 8
        RSPQ_DefineCommand MenuCmd_SetAttrFlags, 4
        RSPQ_DefineCommand MenuCmd_SetViewport, 12
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
        .align 4
        MTX_TOP_UV: 
            .half 0x0001, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0001, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
        .align 4
        CAP_VECTORS:
            # pattern x -y y x
            # 2 patterns per vector
            .half 0x7fff, 0x7fff, 0x8001, 0x7fff, 0x7fff, 0x8001, 0x7fff, 0x7fff
        .align 3
        MTX_STACK_PTR:  .word 0
        MTX_STACK_SIZE: .byte 0
        MTX_STACK_MAX: .byte 10
        MTX_STACK_PTR_UV:  .word 0
        MTX_STACK_SIZE_UV: .byte 0
        MTX_STACK_MAX_UV: .byte 10
        ATTR_FLAGS: .byte 0
        VTX_FX_FLAGS: .byte 0

        .align 2
        VIEWPORT: .half 0, -320*4, 0, -240*4
        .align 4
        VERTEX_POINTS: .space VERTEX_SIZE*2
        NEXT_VERTEX: .byte 0
        
    RSPQ_EndSavedState

.bss
    .align 4
    TMP_MEM:
    VERTEX_INPUT: .space VERTEX_SIZE
    .align 4
    MTX_INPUT: .space 32
    .align 4
RDPQ_TRI_DATA:          .space TRI_VERTEX_SIZE * TRI_VTX_COUNT

#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

.macro vmifxif oi, of, ai, af, bi, bf
    vmudl \of, \af, \bf
    vmadm \of, \ai, \bf
    vmadn \of, \af, \bi
    vmadh \oi, \ai, \bi
    vmadn \of, vzero, vzero
.endm

.macro vmifxi oi, of, ai, af, bi
    vmudn \of, \af, \bi
    vmadh \oi, \ai, \bi
    vmudn \of, vzero, vzero
.endm

.macro vmixif oi, of, ai, bi, bf
    vmudm \of, \ai, \bf
    vmadh \oi, \ai, \bi
    vmadn \of, vzero, vzero
.endm

.macro vmixf oi, of, ai, bf
    vmudm \oi, \ai, \bf
    vmadn \of, vzero, vzero
.endm

.macro vmfxi oi, of, af, bi
    vmudn \of, \af, \bi
    vmadm \oi, vzero, vzero
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
    sb zero, %lo(NEXT_VERTEX)
    jal_and_j WriteNextPoint, RSPQ_Loop

    ##########################################
    # MenuCmd_LineTo
    # 
    # a0: unused
    # a1: xxyy
    # a2: uuww
    # a3: rgba
MenuCmd_LineTo:
    jal WriteNextPoint
    nop

    #define magSqri $v01
    #define magSqrf $v02
    #define pointA  $v03
    #define pointB  $v04
    #define offset  $v05
    #define invMagi $v06
    #define invMagf $v07
    #define vtmp    $v08
    #define offsetNorm $v09
    #define lineWidth  $v10

    lb t6, VERTEX_CLIP_MASK(0)
    lb t7, VERTEX_CLIP_MASK(1)

    and t6, t7, t6

    bne t6, zero, RSPQ_Loop

    li t6, %lo(VERTEX_POINTS)

    llv pointA, 0, t6
    llv pointB, VERTEX_SIZE, t6

    vsub offset, pointA, pointB
    vabs magSqrf, offset, offset

    vmudn magSqrf, magSqrf, magSqrf
    vmadh magSqri, vzero, vzero

    vaddc magSqrf, magSqrf, magSqrf.q1
    vadd magSqri, magSqri, magSqri.q1

    # check if magnitude is zero
    li t7, %lo(VERTEX_INPUT)
    ssv magSqri, 0, 0, t7
    ssv magSqrf, 0, 2, t7
    lw t7, %lo(VERTEX_INPUT)
    beq t7, zero, RSPQ_Loop

    vrsqh vtmp.e0, magSqri.e0
    vrsql invMagf.e0, magSqrf.e0
    vrsqh invMagi.e0, vzero.e0

    vmixif offsetNorm, vtmp, offset, invMagi.e0, invMagf.e0

    # pad out xy, to xyxyxyxy
    li t3, %lo(TMP_MEM)
    slv offsetNorm, 0, 0, t3
    llv offsetNorm, 4, 0, t3
    llv offsetNorm, 8, 0, t3
    llv offsetNorm, 12, 0, t3

    lsv lineWidth, 0, 6, t6
    
    vmov $v16.e0, pointA.e0
    vmov $v16.e2, pointA.e1
    vmov $v16.e4, pointA.e0
    vmov $v16.e6, pointA.e1

    vmixf $v17, $v18, offsetNorm, lineWidth.e0

    jal DrawCap
    li a0, TRI_VTX(0, 0)
    
    vsub offsetNorm, vzero, offsetNorm
    lsv lineWidth, 0, VERTEX_SIZE + 6, t6

    vmov $v16.e0, pointB.e0
    vmov $v16.e2, pointB.e1
    vmov $v16.e4, pointB.e0
    vmov $v16.e6, pointB.e1
    
    vmixf $v17, $v18, offsetNorm, lineWidth.e0

    jal DrawCap
    li a0, TRI_VTX(3, 0)

    li t7, 0xFF
    sw t7, TRI_VTX(0, 4)
    sw t7, TRI_VTX(1, 4)
    sw t7, TRI_VTX(2, 4)
    sw t7, TRI_VTX(3, 4)
    sw t7, TRI_VTX(4, 4)
    sw t7, TRI_VTX(5, 4)
    
    # copy color
    lw t6, VERTEX_RGBA(0)
    sw t6, TRI_VTX(0, 8)
    sw t6, TRI_VTX(1, 8)
    sw t6, TRI_VTX(2, 8)
    
    lw t6, VERTEX_RGBA(1)
    sw t6, TRI_VTX(3, 8)
    sw t6, TRI_VTX(4, 8)
    sw t6, TRI_VTX(5, 8)

    # populate uv
    lh t6, VERTEX_U(0)
    sh t6, TRI_VTX(0, 12)
    sh t6, TRI_VTX(1, 12)
    sh t6, TRI_VTX(2, 12)
    
    lh t6, VERTEX_U(1)
    sh t6, TRI_VTX(3, 12)
    sh t6, TRI_VTX(4, 12)
    sh t6, TRI_VTX(5, 12)
    
    # populate w/winv
    li t6, 0x10000
    sw t6, TRI_VTX(0, 16)
    sw t6, TRI_VTX(0, 20)
    sw t6, TRI_VTX(1, 16)
    sw t6, TRI_VTX(1, 20)
    sw t6, TRI_VTX(2, 16)
    sw t6, TRI_VTX(2, 20)
    sw t6, TRI_VTX(3, 16)
    sw t6, TRI_VTX(3, 20)
    sw t6, TRI_VTX(4, 16)
    sw t6, TRI_VTX(4, 20)
    sw t6, TRI_VTX(5, 16)
    sw t6, TRI_VTX(5, 20)

    lb t7, %lo(ATTR_FLAGS)
    sll t7, t7, 8
    
    li s4, %lo(RDPQ_CMD_STAGING)
    move s3, s4
    li v0, 2   # disable culling
    ori a0, t7, 0xC800
    li a1, TRI_VTX(0, 0)
    li a2, TRI_VTX(1, 0)
    jal RDPQ_Triangle_Send_Async
    li a3, TRI_VTX(3, 0)

    li s4, %lo(RDPQ_CMD_STAGING)
    move s3, s4
    li v0, 2   # disable culling
    ori a0, t7, 0xC800
    li a1, TRI_VTX(0, 0)
    li a2, TRI_VTX(4, 0)
    jal RDPQ_Triangle_Send_Async
    li a3, TRI_VTX(3, 0)

    jal_and_j RDPQ_Triangle_Send_End, RSPQ_Loop

RDPQ_Triangle_Clip:
RDPQ_Triangle_Cull:
    jal_and_j RDPQ_Triangle_Send_End, RSPQ_Loop

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

    li a0, %lo(MTX_TOP)
    li a1, %lo(MTX_INPUT)
    li a2, %lo(MTX_TOP)
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
    # MenuCmd_SetStack
    # a0 max size
    # a1 mtx_addr
MenuCmd_SetStack:
    sb a0, %lo(MTX_STACK_MAX)
    sw a1, %lo(MTX_STACK_PTR)
    j RSPQ_Loop
    sb zero, %lo(MTX_STACK_SIZE)

    ##########################################
    # Does the matrix operation MTX_TOP = MTX_TOP * MTX_INPUT

    #define mtx_topi         $v01
    #define mtx_topf         $v02

    #define input_vtxi       $v03

    #define input_topi       $v03
    #define input_topf       $v04
    #define input_bottomi    $v05
    #define input_bottomf    $v06

    ##########################################
    # MenuCmd_SetAttrFlags
    # a0 flags
MenuCmd_SetAttrFlags:
    j RSPQ_Loop
    sb a0, %lo(ATTR_FLAGS)

    ##########################################
    # MenuCmd_SetViewport
    # a0 unused
    # a1 xmin, xmax
    # a2 ymin, ymax
MenuCmd_SetViewport:
    sw a1, %lo(VIEWPORT + 0)
    j RSPQ_Loop
    sw a2, %lo(VIEWPORT + 4)

    ##########################################
    # MulMtx
    #
    # a0: ptr to left matrix 
    # a1: ptr to right matrix
    # a2: output, can equal left or right
MulMtx:
    lqv mtx_topi, 0x00, a0
    lqv mtx_topf, 0x10, a0
    ldv input_topi, 0, 0x00, a1
    ldv input_topi, 8, 0x00, a1
    ldv input_topf, 0, 0x010, a1
    ldv input_topf, 8, 0x010, a1
    ldv input_bottomi, 0, 0x08, a1
    ldv input_bottomi, 8, 0x08, a1
    ldv input_bottomf, 0, 0x018, a1
    ldv input_bottomf, 8, 0x018, a1

    # matrix multiply
    vmifxif $v08, $v07, input_topi, input_topf, mtx_topi.h0, mtx_topf.h0
    vmifxif $v10, $v09, input_bottomi, input_bottomf, mtx_topi.h1, mtx_topf.h1
    vaddif $v08, $v07, $v08, $v07, $v10, $v09
    
    # store rotate/scale
    # integer half
    slv $v08, 0, 0x00, a2
    slv $v08, 8, 0x08, a2
    # add translate
    vadd $v08, $v08, mtx_topi.h2
    # fixed point half
    slv $v07, 0, 0x10, a2
    slv $v07, 8, 0x18, a2
    # add translate
    vadd $v07, $v07, mtx_topf.h2

    # store translate
    ssv $v08, 4, 0x04, a2
    ssv $v08, 12, 0x0C, a2

    # store translate
    ssv $v07, 4, 0x14, a2
    jr ra
    ssv $v07, 12, 0x1C, a2

WriteNextPoint:
    ##########################################
    # WriteNextPoint
    #
    # a0: unused
    # a1: xxyy
    # a2: uuww
    # a3: rgba

    lb a0, %lo(NEXT_VERTEX)
    xori $at, a0, VERTEX_SIZE
    sb $at, %lo(NEXT_VERTEX)
    addi a0, a0, %lo(VERTEX_POINTS)

    sw a2, 4(a0)
    sw a3, 8(a0)

    sw a1, %lo(VERTEX_INPUT)
    li a1, %lo(MTX_TOP)
    li a2, %lo(VERTEX_INPUT)

    # intentially fall into MulPoint

    ##########################################
    # Does the matrix operation *a0 = MTX_TOP * VERTEX_INPUT
    # a0 output_ptr
    # a1 mtx_ptr
    # a2 input_ptr
MulPoint:
    lqv mtx_topi, 0x00, a1
    lqv mtx_topf, 0x10, a1 

    llv input_vtxi, 0, 0, a2
    llv input_vtxi, 8, 0, a2

    vmifxi $v08, $v07, mtx_topi, mtx_topf, input_vtxi

    vaddc $v07, $v07, $v07.h1
    vadd $v08, $v08, $v08.h1

    vaddc $v07, $v07, mtx_topf.h2
    vadd $v08, $v08, mtx_topi.h2

    ssv $v08, 0, 0, a0
    ssv $v08, 8, 2, a0

    # clipping flags
    # load the pattern x, -x, y, -y
    vand $v07, vzero, vzero
    lsv $v07, 0, 0, a0
    lsv $v07, 4, 2, a0
    vadd $v08, $v07, $v07
    vsub $v07, $v08, $v07.q0

    # pad the culling margin by the line width
    lsv $v08, 0, 6, a0
    vadd $v07, $v07, $v08.e0
    
    li a1, %lo(VIEWPORT)
    ldv $v08, 0, 0, a1

    vlt $v08, $v07, $v08
    cfc2 a1, COP2_CTRL_VCC 
    andi a1, a1, 0xF

    jr ra
    sb a1, 12(a0)

    ##########################################
    # Draws a line end cap
    # $v16 point in the pattern      x0y0x0y0
    # $v17 directioni in the pattern xyxyxyxy
    # $v18 directionf in the pattern xyxyxyxy
    # a0 output vtx pointer
    # stomps t3
DrawCap:
    #define origin     $v16
    #define directioni $v17
    #define directionf $v18
    #define capVector  $v19
    #define resi       $v20
    #define resf       $v21

    li t3, %lo(CAP_VECTORS)
    lqv capVector, 0, 0, t3

    vmudm resi, capVector, directionf
    vmadh resf, capVector, directioni

    vsar resi, COP2_ACC_HI

    # round resi
    vaddc resf, resf, K32768
    vadd resi, resi, vzero

    vadd resi, resi, resi.q1
    vadd resi, resi, origin

    ssv resi, 0, 0, a0
    ssv resi, 4, 2, a0
    
    ssv resi, 8, TRI_VERTEX_SIZE + 0, a0
    ssv resi, 12, TRI_VERTEX_SIZE + 2, a0
    
    jr ra
    nop



    .set at

#include <rsp_rdpq.inc>