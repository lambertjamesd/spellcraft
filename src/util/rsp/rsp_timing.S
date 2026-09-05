
#include <rsp_queue.inc>

.data
    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand WaterCmd_StartTime, 4
        RSPQ_DefineCommand WaterCmd_EndTime, 4
    RSPQ_EndOverlayHeader

    RSPQ_BeginSavedState
        .align 4
        TIMERS:
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
            .word 0, 0, 0, 0
    RSPQ_EndSavedState

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
WaterCmd_EndTime:
    andi a0, 0x3F
    mfc0 t0, COP0_DP_CLOCK
    lw t1, %lo(TIMERS)(a0)
    sub t0, t1
    j RSPQ_Loop
    sw t0, %lo(TIMERS)(a0)