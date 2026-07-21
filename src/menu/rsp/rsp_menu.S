
#include <rsp_queue.inc>
#include "rsp_menu.inc"

.data

# fmt xxyy uuww rgba c000
# xxyy - position                   (0, 2)
# uu - u of uv texture coordinate   (4)
# ww - width of the line            (6)
# rgba - color                      (8)
# c - clipping mask                 (12)

#define VERTEX_INPUT_SIZE   12
#define VERTEX_SIZE         16

#define VERTEX_RGBA(index)     (%lo(VERTEX_POINTS) + 8 + index * VERTEX_SIZE)
#define VERTEX_U(index)     (%lo(VERTEX_POINTS) + 4 + index * VERTEX_SIZE)
#define VERTEX_WIDTH(index)     (%lo(VERTEX_POINTS) + 6 + index * VERTEX_SIZE)
#define VERTEX_CLIP_MASK(index)     (%lo(VERTEX_POINTS) + 12 + index * VERTEX_SIZE)

#define TRANSFORM_SIZE      32
#define TRANSFORM_SIZE_LOG2 5

#define TRI_VTX_SIZE_LOG2   5
#define TRI_VERTEX_SIZE     32

# fmt
# xxyy - position (0)
# ???? - (4)
# rgba - (8)
# uuvv - (12)
# wwww - (16)
# winv - (20)
# c - clipping mask (24)

#define TRI_VTX(index, offset)     (%lo(RDPQ_TRI_DATA) + offset + index * TRI_VERTEX_SIZE)

#define TRI_POS         0
#define TRI_POS_X       0
#define TRI_POS_Y       2
#define TRI_RGBA        8
#define TRI_UV          12
#define TRI_CLIP        24

#define INPUT_POS         0
#define INPUT_UV          4
#define INPUT_RGBA        8

#define ROUND_4_OFFSET    2
#define ROUND_4_MASK      0xFFFC

#define TRI_VTX_EMPTY   .half 0, 0; .word 0xFF; .byte 255, 255, 255, 255; .half 0, 0; .word 0x00010000, 0x00010000; .word 0, 0;

    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MenuCmd_MoveTo, 16
        RSPQ_DefineCommand MenuCmd_LineTo, 16
        RSPQ_DefineCommand MenuCmd_Mtx, 8
        RSPQ_DefineCommand MenuCmd_MtxPop, 4
        RSPQ_DefineCommand MenuCmd_SetStack, 12
        RSPQ_DefineCommand MenuCmd_SetAttrFlags, 4
        RSPQ_DefineCommand MenuCmd_SetViewport, 12
        RSPQ_DefineCommand MenuCmd_SetUVFx, 4
        RSPQ_DefineCommand MenuCmd_VTX, 8
        RSPQ_DefineCommand MenuCmd_Tri, 4
        RSPQ_DefineCommand MenuCmd_RelativeFillRect, 8
        RSPQ_DefineCommand MenuCmd_RelativeTexRect, 16
        RSPQ_DefineCommand MenuCmdWrite4, 8
        RSPQ_DefineCommand MenuColorTable, 4
        # set color table
        # color
        # fill rect
        # fixed rect
        # fixed rect array
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
        .align 4
        MTX_TOP_UV: 
            .half 0x0001, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0001, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
            .half 0x0000, 0x0000, 0x0000, 0x0000
        .align 3
        MTX_STACK_PTR_UV:  .word 0
        MTX_STACK_SIZE_UV: .byte 0
        MTX_STACK_MAX_UV: .byte 10
        .align 4
        CAP_VECTORS:
            # pattern x -y y x
            # 2 patterns per vector
            .half 0x7fff, 0x7fff, 0x8001, 0x7fff, 0x7fff, 0x8001, 0x7fff, 0x7fff
        .align 4
        ATTR_FLAGS: .byte 0
        VTX_FX_FLAGS: .byte 0

        .align 2
        VIEWPORT_CLIP: .half 0, -320*4, 0, -240*4
        VIEWPORT: .half 0, 320*4, 0, 240*4
        .align 4
        VERTEX_POINTS: .space VERTEX_SIZE*2
        NEXT_VERTEX: .byte 0

        .align 4
        RDPQ_TRI_DATA:
            .rept MENU_TRI_VTX_COUNT
                TRI_VTX_EMPTY
            .endr

        .align 4
        SEGMENT_TABLE: .space 4 * 16
                
    RSPQ_EndSavedState

.bss
    .align 4
    TMP_MEM:
    # fmt xxyy uuvv rgba
    MTX_INPUT:  .space TRANSFORM_SIZE

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
    jal WriteNextPoint
    sb zero, %lo(NEXT_VERTEX)

    # load line width as the margin
    lsv $v20, 0, 6, a0
    addi a1, a0, 12

    jal_and_j ClipPoint, RSPQ_Loop

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
    
    # load line width as the margin
    lsv $v20, 0, 6, a0
    jal ClipPoint
    addi a1, a0, 12


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
    li t7, %lo(TMP_MEM)
    ssv magSqri, 0, 0, t7
    ssv magSqrf, 0, 2, t7
    lw t7, %lo(TMP_MEM)
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

    li $at, %lo(VTX_FX_FLAGS)
    beq $at, zero, MenuCmd_LineTo_SkipFX
    nop

    jal ApplyUVEffects
    li a0, TRI_VTX(0, 0)
    
    jal ApplyUVEffects
    li a0, TRI_VTX(1, 0)
    
    jal ApplyUVEffects
    li a0, TRI_VTX(3, 0)
    
    jal ApplyUVEffects
    li a0, TRI_VTX(4, 0)

    #undef offset

MenuCmd_LineTo_SkipFX:
    
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

    jal RDPQ_Triangle_Send_End
    nop

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
    #define stack_ptr   s1

    andi $at, a0, MENU_MTX_UV
    beq $at, zero, MenuCmd_Mtx_Start
    addi stack_ptr, zero, 0

    addi stack_ptr, zero, %lo(MTX_TOP_UV - MTX_TOP)

MenuCmd_Mtx_Start:
    andi $at, a0, MENU_MTX_PUSH
    beq $at, zero, MenuCmd_Mtx_SkipPush

    lb t2, %lo(MTX_STACK_SIZE)(stack_ptr)
    lb t3, %lo(MTX_STACK_MAX)(stack_ptr)
    beq t2, t3, RSPQ_Loop
    addi t2, t2, 1
    sb t2, %lo(MTX_STACK_SIZE)(stack_ptr)

    addi dmem_address, stack_ptr, %lo(MTX_TOP)
    lw dram_address, %lo(MTX_STACK_PTR)(stack_ptr)
    li dma_size, DMA_SIZE(32, 1)
    jal DMAOut
    li row_stride, 0

    addi dram_address, dram_address, TRANSFORM_SIZE
    sw dram_address, %lo(MTX_STACK_PTR)(stack_ptr)
MenuCmd_Mtx_SkipPush:
    andi $at, a0, MENU_MTX_MUL
    beq $at, zero, MenuCmd_Mtx_SkipMul
    
    li dmem_address, %lo(MTX_INPUT)
    addi dram_address, a1, 0
    li dma_size, DMA_SIZE(32, 1)
    jal DMAIn
    li row_stride, 0

    addi a0, stack_ptr, %lo(MTX_TOP)
    li a1, %lo(MTX_INPUT)
    addi a2, stack_ptr, %lo(MTX_TOP)
    jal_and_j MulMtx, RSPQ_Loop

MenuCmd_Mtx_SkipMul:
    addi dmem_address, stack_ptr, %lo(MTX_TOP)
    addi dram_address, a1, 0
    li dma_size, DMA_SIZE(32, 1)
    li row_stride, 0
    jal_and_j DMAIn, RSPQ_Loop

    ##########################################
    # MenuCmd_Pop
    # a0 count
MenuCmd_MtxPop:
    srl $at, a0, 8
    andi $at, $at, 0x1
    beq $at, zero, MenuCmd_MtxPop_Start
    addi stack_ptr, zero, 0

    addi stack_ptr, zero, %lo(MTX_TOP_UV - MTX_TOP)

MenuCmd_MtxPop_Start:
    
    andi a0, a0, 0xFF
    lb t2, %lo(MTX_STACK_SIZE)(stack_ptr)
    sub t3, t2, a0

    # prevent underflow
    bgez t3, MenuCmd_MtxPopDoPop
    nop
    addi a0, t2, 0
    xor t3, t3, t3
MenuCmd_MtxPopDoPop:
    # check if count is zero, do nothing
    beq a0, zero, RSPQ_Loop
    sll a0, a0, TRANSFORM_SIZE_LOG2

    sb t3, %lo(MTX_STACK_SIZE)(stack_ptr)
    lw dram_address, %lo(MTX_STACK_PTR)(stack_ptr)
    sub dram_address, dram_address, a0
    sw dram_address, %lo(MTX_STACK_PTR)(stack_ptr)

    # load prev matrix
    addi dmem_address, stack_ptr, %lo(MTX_TOP)
    li dma_size, DMA_SIZE(32, 1)
    li row_stride, 0
    jal_and_j DMAIn, RSPQ_Loop

    
    ##########################################
    # MenuCmd_SetStack
    # a0 max size
    # a1 mtx_addr
    # a2 mtx_uv_addr
MenuCmd_SetStack:
    sb a0, %lo(MTX_STACK_MAX)
    sw a1, %lo(MTX_STACK_PTR)
    sb zero, %lo(MTX_STACK_SIZE)

    srl a0, a0, 8
    sb a0, %lo(MTX_STACK_MAX_UV)
    sw a1, %lo(MTX_STACK_PTR_UV)
    j RSPQ_Loop
    sb zero, %lo(MTX_STACK_SIZE_UV)

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
    sw a1, %lo(VIEWPORT)
    sw a2, %lo(VIEWPORT + 4)

    xori a1, a1, 0xFFFF
    xori a2, a2, 0xFFFF
    addi a1, a1, 1
    addi a2, a2, 1
    
    sw a1, %lo(VIEWPORT_CLIP + 0)
    sw a2, %lo(VIEWPORT_CLIP + 4)

    j RSPQ_Loop
    nop

    ##########################################
    # MenuCmd_SetUVFx
    # a0 flags
MenuCmd_SetUVFx:
    j RSPQ_Loop
    sb a0, %lo(VTX_FX_FLAGS)


    ##########################################
    # MenuCmd_VTX
    # a0 offset:8 | count:8
    # a1 vtx ptr
MenuCmd_VTX:
    #define input_ptr   s5
    #define vtx_ptr     s6
    #define count       t3
    #define counter     t6
    #define offset      t7
    
    andi count, a0, 0xFF
    beq count, zero, RSPQ_Loop
    srl offset, a0, 8
    andi offset, offset, 0xFF

    # dma_size = count * 12 - 1
    sll t5, count, 3
    sll t6, count, 2
    add dma_size, t5, t6
    addi dma_size, dma_size, -1

    li vtx_ptr, TRI_VTX(0, 0)
    sll offset, offset, TRI_VTX_SIZE_LOG2
    add vtx_ptr, vtx_ptr, offset

    addi dram_address, a1, 0
    addi row_stride, zero, 0
    jal DMAIn
    addi dmem_address, vtx_ptr, 0

    addi input_ptr, vtx_ptr, 0

    # Move pointer to the end of lists
    # and work backwards
    addi counter, count, -1
MenuCmd_VTX_IncLoop:
    addi counter, counter, -1
    addi vtx_ptr, vtx_ptr, TRI_VERTEX_SIZE
    bne counter, zero, MenuCmd_VTX_IncLoop
    addi input_ptr, input_ptr, VERTEX_INPUT_SIZE

    addi counter, count, 0
MenuCmd_VTX_NextVtx:
    beq counter, zero, MenuCmd_VTX_CheckEffects

    # transform point
    addi a0, vtx_ptr, TRI_POS
    li a1, %lo(MTX_TOP)
    jal MulPoint
    addi a2, input_ptr, INPUT_POS

    # copy uv
    # copy rgba
    lw t4, INPUT_UV(input_ptr)
    lw t5, INPUT_RGBA(input_ptr)

    sw t4, TRI_UV(vtx_ptr)
    sw t5, TRI_RGBA(vtx_ptr)

    # check clipping
    vadd $v20, vzero, vzero
    addi a0, vtx_ptr, 0
    jal ClipPoint
    addi a1, vtx_ptr, TRI_CLIP

    addi vtx_ptr, vtx_ptr, -TRI_VERTEX_SIZE
    addi input_ptr, input_ptr, -VERTEX_INPUT_SIZE

    j MenuCmd_VTX_NextVtx
    addi counter, counter, -1

MenuCmd_VTX_CheckEffects:
    li $at, %lo(VTX_FX_FLAGS)
    beq $at, zero, RSPQ_Loop

    li input_ptr, TRI_VTX(0, 0)
    add input_ptr, input_ptr, offset
MenuCmd_VTX_NextEffectVtx:
    beq count, zero, RSPQ_Loop
    addi count, count, -1

    jal ApplyUVEffects
    addi a0, input_ptr, 0

    j MenuCmd_VTX_NextEffectVtx
    addi input_ptr, input_ptr, TRI_VERTEX_SIZE

    #undef count

    ##########################################
    # MenuCmd_Tri
    # a0 a:8 | b:8 | c:8
MenuCmd_Tri:

    # load pointers to triangles
    andi a1, a0, 0xFF
    sll a1, a1, TRI_VTX_SIZE_LOG2
    addi a1, a1, %lo(RDPQ_TRI_DATA)

    srl a2, a0, 8 - TRI_VTX_SIZE_LOG2
    andi a2, a2, 0xFF << TRI_VTX_SIZE_LOG2
    addi a2, a2, %lo(RDPQ_TRI_DATA)
    
    srl a3, a0, 16 - TRI_VTX_SIZE_LOG2
    andi a3, a3, 0xFF << TRI_VTX_SIZE_LOG2
    addi a3, a3, %lo(RDPQ_TRI_DATA)
    
    # check clippping
    lb t1, TRI_CLIP(a1)
    lb t2, TRI_CLIP(a2)
    lb t3, TRI_CLIP(a3)

    and t1, t1, t2
    and t1, t1, t3

    bne t1, zero, RSPQ_Loop

    # prepare triangle command
    lb t7, %lo(ATTR_FLAGS)
    sll t7, t7, 8

    li s4, %lo(RDPQ_CMD_STAGING)
    move s3, s4
    li v0, 2   # disable culling
    jal RDPQ_Triangle_Send_Async
    ori a0, t7, 0xC800

    j RSPQ_Loop
    nop

    jal_and_j RDPQ_Triangle_Send_End, RSPQ_Loop

    ##########################################
    # MenuCmd_RelativeFillRect
    # a0 x2:12 y2:12
    # a1 vtx:5 pad:3 x1:12 y1:12 
MenuCmd_RelativeFillRect:
    jal ClipRect
    nop

    li $at, 0x36
    sb $at, %lo(RDPQ_CMD_STAGING)

    li s4, %lo(RDPQ_CMD_STAGING)
    li s3, %lo(RDPQ_CMD_STAGING+8)

    jal_and_j RDPQ_Send, RSPQ_Loop
    
    ##########################################
    # MenuCmd_RelativeTexRect
    # a0 x2:12 y2:12
    # a1 vtx:5 pad:3 x1:12 y1:12 
    # a2 s,t
    # a3 dsdx, dtdy
MenuCmd_RelativeTexRect:
    jal ClipRect
    nop
    
    li $at, 0x24
    beq v0, zero, MenuCmd_RelativeTexRectSkipUV
    sb $at, %lo(RDPQ_CMD_STAGING)

    # load dsdx and dtdy into vector registers
    mtc2 a3, $v01.e2
    srl s7, a3, 16
    mtc2 s7, $v01.e0
    # scale dsdx by clip amount         ; # load st into registers
    vmudm $v02, $v01, $v20              ; mtc2 a2, $v05.e2
    vmadl $v03, vzero, vzero            ; srl s7, a2, 16
    # bit shift to the right by 7
    vmudl $v04, $v03, K512              ; mtc2 s7, $v05.e0
    vmadm $v04, $v02, K512              ;
    vmadl $v04, vzero, vzero            ;
    
    # adjust st
    vadd $v05, $v05, $v04

    mfc2 a2, $v05.e0
    mfc2 s6, $v05.e2
    sll a2, a2, 16
    andi s6, s6, 0xFFFF
    or a2, a2, s6

MenuCmd_RelativeTexRectSkipUV:

    sw a2, %lo(RDPQ_CMD_STAGING+8)
    sw a3, %lo(RDPQ_CMD_STAGING+12)

    li s4, %lo(RDPQ_CMD_STAGING)
    li s3, %lo(RDPQ_CMD_STAGING+16)

    jal_and_j RDPQ_Send, RSPQ_Loop

    ##########################################
    # MenuCmdWrite4
    # a0 relative_addr:1 dmem_addr:12
    # a1 payload
MenuCmdWrite4:
    andi $at, a0, 0x1000

    beq $at, zero, MenuCmdDoWrite
    andi s7, a0, 0xFFF

    sll s6, a1, 22
    andi s6, s6, 0x3C

    lw s5, %lo(SEGMENT_TABLE)(s6)

    li s4, 0xFFFFFF
    and s4, s4, a1
    add a1, s4, s5
    
MenuCmdDoWrite:
    j RSPQ_Loop
    sw a1, 0(s7)

    ##########################################
    # MenuColorTable
    # a0 color_type:8 color_index:16
MenuColorTable:
    j RSPQ_Loop
    nop


    ##########################################
    # ClipRect
    # a0 x2:12 y2:12
    # a1 vtx:5 pad:3 x1:12 y1:12 
    # result $v20 amount s and t need to be modified by
ClipRect:
    li s1, %lo(VIEWPORT)
    srl s0, a1, 27 - TRI_VTX_SIZE_LOG2
    andi s0, s0, %lo(~(TRI_VERTEX_SIZE - 1))
    addi s0, s0, %lo(RDPQ_TRI_DATA)
    
    li s6, ROUND_4_OFFSET
    li s7, ROUND_4_MASK
    mtc2 s6, $v14.e0
    mtc2 s7, $v15.e0

    #define rel_point       $v10
    #define constrained     $v11
    #define viewport        $v12
    #define res_neg_clip    $v20

    # relative to
    lsv rel_point, 0, 0, s0
    lsv rel_point, 4, 2, s0

    # viewport
    ldv viewport, 0, 0, s1

    # load 
    # x2
    sll s1, a0, 8
    sra s1, s1, 20
    mtc2 s1, constrained.e1          ; vadd rel_point, rel_point, $v14.e0

    # y2
    sll s1, a0, 20
    sra s1, s1, 20
    mtc2 s1, constrained.e3             
    
    # x1
    sll s1, a1, 8
    sra s1, s1, 20
    mtc2 s1, constrained.e0          ; vand rel_point, rel_point, $v15.e0

    # y1
    sll s1, a1, 20                      
    sra s1, s1, 20
    mtc2 s1, constrained.e2

    # offset points
    vadd constrained, constrained, rel_point.q0
    
    # calculate clip offset for uv
    vsub res_neg_clip, viewport, constrained

    # constrain lower bound
    vge constrained, constrained, viewport.q0
    vge res_neg_clip, res_neg_clip, vzero
    cfc2 v0, COP2_CTRL_VCC

    # constrain upper bound
    vlt constrained, constrained, viewport.q1
    andi v0, v0, 0x5

    # check if rectangle is fully clipped
    vge $v13, constrained, constrained.q1
    cfc2 s0, COP2_CTRL_VCC
    andi s0, s0, 0x5
    bne s0, zero, RSPQ_Loop

    # store result
    mfc2 s0, constrained.e0
    mfc2 s1, constrained.e1
    mfc2 s2, constrained.e2
    mfc2 s3, constrained.e3

    andi s0, s0, 0xFFF
    andi s1, s1, 0xFFF
    andi s2, s2, 0xFFF
    andi s3, s3, 0xFFF
    
    sll s0, s0, 12
    sll s1, s1, 12
    
    or s0, s0, s2
    or s1, s1, s3
    sw s0, %lo(RDPQ_CMD_STAGING + 4)

    jr ra
    sw s1, %lo(RDPQ_CMD_STAGING)

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

    sw a1, %lo(TMP_MEM)
    li a1, %lo(MTX_TOP)
    li a2, %lo(TMP_MEM)

    # intentially fall into MulPoint

    ##########################################
    # Does the matrix operation *a0 = MTX_TOP * *input_ptr
    # a0 output_ptr
    # a1 mtx_ptr
    # a2 input_ptr
    #
    # stomps $v01 $v02 $v03 $v08 $v07
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
    jr ra
    ssv $v08, 8, 2, a0

    ##########################################
    # clips the
    # a0 vtx_ptr
    # a1 output_ptr (byte)
    # $v20 margin
ClipPoint:

    # clipping flags
    # load the pattern x, -x, y, -y
    vand $v07, vzero, vzero
    lsv $v07, 0, 0, a0
    lsv $v07, 4, 2, a0
    vadd $v08, $v07, $v07
    vsub $v07, $v08, $v07.q0

    # pad the culling margin by the margin
    vadd $v07, $v07, $v20.e0
    
    li a0, %lo(VIEWPORT_CLIP)
    ldv $v08, 0, 0, a0

    vlt $v08, $v07, $v08
    cfc2 a0, COP2_CTRL_VCC 
    andi a0, a0, 0xF

    jr ra
    sb a0, 0(a1)

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

    ##########################################
    # Applies UV effects
    # a0 ptr to tri vertex
    # stomps $at, $v01 a0, a1, a2
ApplyUVEffects:
    lb t0, %lo(VTX_FX_FLAGS)                       ; llv $v01, 0, TRI_POS, a0
    andi $at, t0, MENU_FX_UV_GEN

    beq t0, zero, ApplyUVCheckTransform            ; vmudh $v01, $v01, K8
    
    slv $v01, 0, TRI_UV, a0

ApplyUVCheckTransform:
    andi $at, t0, MENU_FX_UV_MTX
    beq t0, zero, ApplyUVEffects_End

    # a0 output_ptr
    # a1 mtx_ptr
    # a2 input_ptr
    addi a0, a0, TRI_UV
    addi a2, a0, 0
    j MulPoint
    li a1, %lo(MTX_TOP_UV)    

ApplyUVEffects_End:
    jr ra
    nop

    .set at

#include <rsp_rdpq.inc>