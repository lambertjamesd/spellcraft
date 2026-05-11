#include "rspq.h"

struct gross_rspq_block_duplicate {
    uint32_t nesting_level;     ///< Nesting level of the block
    void *rdp_block;    ///< Option RDP static buffer (with RDP commands)
    void *atexit;    ///< List of callbacks to call upon freeing the block
    uint32_t cmds[];            ///< Block contents (commands)
};

static uint8_t command_size[] = {
    [0x00] = 0, // RSPQCmd_WaitNewInput 
    [0x01] = 4, // RSPQCmd_Noop 
    [0x02] = 4, // RSPQCmd_Jump 
    [0x03] = 8, // RSPQCmd_Call 
    [0x04] = 4, // RSPQCmd_Ret 
    [0x05] = 16, // RSPQCmd_Dma 
    [0x06] = 4, // RSPQCmd_WriteStatus 
    [0x07] = 12, // RSPQCmd_SwapBuffers 
    [0x08] = 8, // RSPQCmd_TestWriteStatus 
    [0x09] = 12, // RSPQCmd_RdpSetBuffer 
    [0x0A] = 4, // RSPQCmd_RdpAppendBuffer 
    
    [0x10] = 8, // T3DCmd_TriDraw
    [0x11] = 16, // T3DCmd_SetScreenSize
    [0x12] = 8, // T3DCmd_MatrixStack
    [0x13] = 8, // T3DCmd_SetWord
    [0x14] = 12, // T3DCmd_VertLoad
    [0x15] = 16, // T3DCmd_LightSet
    [0x16] = 8, // T3DCmd_RenderMode
    [0x17] = 4, // T3DCmd_MatProjectionSet
    [0x18] = 16, // T3DCmd_Patch
    [0x19] = 4, // T3DCmd_SetFogState
    [0x1A] = 4, // T3DCmd_TriSync
    [0x1B] = 8, // T3DCmd_TriDraw_Strip
    [0x1C] = 8, // T3DCmd_TriDraw_Seq
    
    [0xC0] = 8, // RDPQCmd_Passthrough8 NOOP
    [0xC1] = 8, // RDPQCmd_SetLookupAddress Set lookup address
    [0xC2] = 8, // RDPQCmd_RectEx Fill Rectangle (esclusive bounds)
    [0xC3] = 16, // RDPQCmd_ClearZBuffer CLear Z-Buffer
    [0xC4] = 16, // RDPQCmd_ResetMode Reset Mode (set mode standard)
    [0xC5] = 8, // RDPQCmd_SetCombineMode_2Pass SET_COMBINE_MODE (two pass)
    [0xC6] = 4, // RDPQCmd_PushMode Push Mode
    [0xC7] = 4, // RDPQCmd_PopMode Pop Mode
    [0xC8] = 32, // RDPQCmd_PassthroughTriangle Filled
    [0xC9] = 48, // RDPQCmd_PassthroughTriangle Filled ZBuffered
    [0xCA] = 96, // RDPQCmd_PassthroughTriangle Textured
    [0xCB] = 112, // RDPQCmd_PassthroughTriangle Textured ZBuffered
    [0xCC] = 96, // RDPQCmd_PassthroughTriangle Shaded
    [0xCD] = 112, // RDPQCmd_PassthroughTriangle Shaded ZBuffered
    [0xCE] = 160, // RDPQCmd_PassthroughTriangle Shaded Textured
    [0xCF] = 176, // RDPQCmd_PassthroughTriangle Shaded Textured ZBuffered

    [0xD0] = 16, // RDPQCmd_RectEx Texture Rectangle (esclusive bounds)
    [0xD1] = 4, // RDPQCmd_SetDebugMode Set Debug mode
    [0xD2] = 8, // RDPQCmd_SetScissorEx Set Scissor (exclusive bounds)
    [0xD3] = 8, // RDPQCmd_SetPrimColorComponent Set Primimive Color Component (minlod or primlod or rgba)
    [0xD4] = 12, // RDPQCmd_ModifyOtherModes Modify SOM
    [0xD5] = 8, // RSPQCmd_Noop 
    [0xD6] = 8, // RDPQCmd_SetFillColor32 
    [0xD7] = 8, // RSPQCmd_Noop 
    [0xD8] = 8, // RDPQCmd_SetBlendingMode Set Blending Mode
    [0xD9] = 8, // RDPQCmd_SetFogMode Set Fog Mode
    [0xDA] = 4, // RDPQCmd_RdpWaitIdle RDP Wait Idle
    [0xDB] = 16, // RDPQCmd_SetCombineMode_1Pass SET_COMBINE_MODE (one pass)
    [0xDC] = 4, // RDPQCmd_AutoTmem_SetAddr AutoTmem_SetAddr
    [0xDD] = 8, // RDPQCmd_AutoTmem_SetTile AutoTmem_SetTile
    [0xDE] = 4, // RDPQCmd_Triangle Triangle (assembled by RSP)
    [0xDF] = 28, // RDPQCmd_TriangleData Set Triangle Data

    [0xE0] = 8, // RSPQCmd_Noop 
    [0xE1] = 8, // RSPQCmd_Noop 
    [0xE2] = 8, // RSPQCmd_Noop 
    [0xE3] = 8, // RSPQCmd_Noop 
    [0xE4] = 16, // RDPQCmd_Passthrough16 TEXTURE_RECTANGLE
    [0xE5] = 16, // RDPQCmd_Passthrough16 TEXTURE_RECTANGLE_FLIP
    [0xE6] = 8, // RDPQCmd_Passthrough8 SYNC_LOAD
    [0xE7] = 8, // RDPQCmd_Passthrough8 SYNC_PIPE
    [0xE8] = 8, // RDPQCmd_Passthrough8 SYNC_TILE
    [0xE9] = 8, // RDPQCmd_SyncFull SYNC_FULL
    [0xEA] = 8, // RDPQCmd_Passthrough8 SET_KEY_GB
    [0xEB] = 8, // RDPQCmd_Passthrough8 SET_KEY_R
    [0xEC] = 8, // RDPQCmd_Passthrough8 SET_CONVERT
    [0xED] = 8, // RDPQCmd_Passthrough8 SET_SCISSOR
    [0xEE] = 8, // RDPQCmd_Passthrough8 SET_PRIM_DEPTH
    [0xEF] = 8, // RDPQCmd_SetOtherModes SET_OTHER_MODES
    [0xF0] = 8, // RDPQCmd_Passthrough8 LOAD_TLUT
    [0xF1] = 8, // RDPQCmd_Passthrough8 RDPQ_DEBUG (debugging command)
    [0xF2] = 8, // RDPQCmd_Passthrough8 SET_TILE_SIZE
    [0xF3] = 8, // RDPQCmd_Passthrough8 LOAD_BLOCK
    [0xF4] = 8, // RDPQCmd_Passthrough8 LOAD_TILE
    [0xF5] = 8, // RDPQCmd_Passthrough8 SET_TILE
    [0xF6] = 8, // RDPQCmd_Passthrough8 FILL_RECTANGLE
    [0xF7] = 8, // RDPQCmd_Passthrough8 SET_FILL_COLOR
    [0xF8] = 8, // RDPQCmd_Passthrough8 SET_FOG_COLOR
    [0xF9] = 8, // RDPQCmd_Passthrough8 SET_BLEND_COLOR
    [0xFA] = 8, // RDPQCmd_SetPrimColor SET_PRIM_COLOR
    [0xFB] = 8, // RDPQCmd_Passthrough8 SET_ENV_COLOR
    [0xFC] = 8, // RDPQCmd_Passthrough8 SET_COMBINE_MODE
    [0xFD] = 8, // RDPQCmd_SetFixupImage SET_TEXTURE_IMAGE
    [0xFE] = 8, // RDPQCmd_SetFixupImage SET_Z_IMAGE
    [0xFF] = 8, // RDPQCmd_SetColorImage SET_COLOR_IMAGE
};

static uint8_t overlay_mapping[16] = {
    [0x0] = 0x0,
    [0x1] = 0x1,
    [0x2] = 0x2,
    [0x3] = 0x3,
    [0x4] = 0x4,
    [0x5] = 0x5,
    [0x6] = 0x6,
    [0x7] = 0x7,
    [0x8] = 0x8,
    [0x9] = 0x9,
    [0xA] = 0xA,
    [0xB] = 0xB,
    [0xC] = 0xC,
    [0xD] = 0xC,
    [0xE] = 0xC,
    [0xF] = 0xC,
};

#define MAX_DISPLAY_LIST_SIZE       1000

uint32_t rspq_count_overlay_switches_cmd(uint32_t* cmd, uint32_t* ending_overlay) {
    uint32_t result = 0;

    for (int iteration = 0; iteration < MAX_DISPLAY_LIST_SIZE; iteration += 1) {
        uint32_t command_id = *cmd >> 24;
        
        if (command_id == 0x02) {
            // RSPQCmd_Jump
            uint32_t addr = (int32_t)KSEG0_START_ADDR + (*cmd & 0xFFFFFF);
            cmd = (uint32_t*)addr;
        } else if (command_id == 0x03) {
            uint32_t addr = (int32_t)KSEG0_START_ADDR + (*cmd & 0xFFFFFF);
            result += rspq_count_overlay_switches_cmd((uint32_t*)addr, ending_overlay);
            cmd += 2;
        } else if (command_id == 0x04) {
            return result;
        } else {
            uint8_t cmd_size = command_size[command_id];

            if (!cmd_size) {
                debugf("cmd %02x has no size\n", command_id);
            }

            assert(cmd_size);
            cmd += cmd_size >> 2;

            uint32_t overlay_id = overlay_mapping[command_id >> 4];

            if (*ending_overlay != overlay_id && overlay_id != 0) {
                debugf(" %02x -> %02x ", *ending_overlay, overlay_id);
                result += 1;
                *ending_overlay = overlay_id;
            }
        }
    }

    assert(false);
    return result;
}

uint32_t rspq_count_overlay_switches(rspq_block_t* block, uint32_t* ending_overlay) {
    *ending_overlay = 0;
    return rspq_count_overlay_switches_cmd(&((struct gross_rspq_block_duplicate*)block)->cmds[0], ending_overlay);
}