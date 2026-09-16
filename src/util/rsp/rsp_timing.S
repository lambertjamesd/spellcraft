
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_StartTime, 4
        RSPQ_DefineCommand WaterCmd_EndTime, 8
    RSPQ_EndOverlayHeader

    RSPQ_BeginSavedState
        .align 4
        TIMERS:
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
        TIMER_OUT:
            .word 0, 0
    RSPQ_EndSavedState
    
#define dma_size     t0
#define row_stride   t1
#define dram_address s0
#define dmem_address s4

.text    
    ##########################################
    # WaterCmd_Profile
    # 
    # a0: timer offset
WaterCmd_StartTime:
    andi a0, 0x3F
    mfc0 t0, COP0_DP_CLOCK
    j RSPQ_Loop
    sw t0, %lo(TIMERS)(a0)
    
    ##########################################
    # WaterCmd_Profile
    # 
    # a0: timer offset
    # a1: result ptr
WaterCmd_EndTime:
    andi a0, 0x3F
    mfc0 t0, COP0_DP_CLOCK
    lw t1, %lo(TIMERS)(a0)
    sub t0, t1

    beq a1, zero, RSPQ_Loop

    sw t0, %lo(TIMER_OUT)

    move dram_address, a1
    li dmem_address, %lo(TIMER_OUT)
    li row_stride, 0
    li dma_size, 7
    jal_and_j DMAOut, RSPQ_Loop