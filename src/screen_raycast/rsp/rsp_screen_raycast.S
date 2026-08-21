
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand ScreenRaycast_CheckPixel, 4
        RSPQ_DefineCommand ScreenRaycast_CheckChanged, 8
        RSPQ_DefineCommand ScreenRaycast_ReadEntity, 4
    RSPQ_BeginSavedState
        .align 4
            PIXEL_ID: .word 0
            LAST_COLOR: .word 0
    RSPQ_EndSavedState
.bss
    .align 4
        COLOR_BUFFER: .word 0, 0, 0, 0

.text

#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

    ##########################################
    # ScreenRaycast_CheckPixel
    # 
    # a0: pixel_addr
ScreenRaycast_CheckPixel:  
    jal ScreenRaycast_RDPWait
    nop

    jal ScreenRaycast_ReadPixel
    nop

    lh t2, 0(dmem_address)
    j RSPQ_Loop
    sh t2, %lo(LAST_COLOR)

    
    ##########################################
    # ScreenRaycast_CheckPixel
    # 
    # a0: pixel_addr
    # a1: id
ScreenRaycast_CheckChanged:
    jal ScreenRaycast_RDPWait
    nop

    jal ScreenRaycast_ReadPixel
    lh t2, %lo(LAST_COLOR)
    
    lh t3, 0(dmem_address)

    beq t2, t3, RSPQ_Loop
    sh t3, %lo(PIXEL_ID)
    j RSPQ_Loop
    sw a1, %lo(PIXEL_ID)

    ##########################################
    # ScreenRaycast_CheckPixel
    # 
    # a0: ouput_addr
ScreenRaycast_ReadEntity:    
    # remove high byte
    sll dram_address, a0, 8
    srl dram_address, 8
    li dmem_address, %lo(PIXEL_ID)
    li row_stride, 0
    jal DMAOut
    li dma_size, DMA_SIZE(8, 1);

    j RSPQ_Loop
    sw zero, %lo(PIXEL_ID)

    ##########################################
    # ScreenRaycast_CheckPixel
    # 
    # a0: pixel_addr
ScreenRaycast_ReadPixel:
    # remove high byte
    sll dram_address, a0, 8
    srl dram_address, 8
    li dmem_address, %lo(COLOR_BUFFER)
    li dma_size, DMA_SIZE(8, 1);
    j DMAIn
    li row_stride, 0

ScreenRaycast_RDPWait:
    mfc0 t2, COP0_DP_STATUS
    andi t2, DP_STATUS_BUSY | DP_STATUS_DMA_BUSY
    bnez t2, ScreenRaycast_RDPWait
    nop
    jr ra
    nop

