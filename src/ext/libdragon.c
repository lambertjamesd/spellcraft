#include "libdragon.h"

#define _carg(value, mask, shift) (((uint32_t)((value) & (mask))) << (shift))
#define TMEM_PALETTE_ADDR   0x800

void __rdpq_write8(uint32_t cmd_id, uint32_t arg0, uint32_t arg1);

void rdpq_write_other_modes_raw(uint32_t w0, uint32_t w1) {
    __rdpq_write8(RDPQ_CMD_SET_OTHER_MODES, w0, w1);
}


void rdpq_tex_upload_tlut_raw(uint16_t *tlut, int color_idx, int num_colors) {
    __rdpq_write8(RDPQ_CMD_SET_TEXTURE_IMAGE, 
        _carg(FMT_RGBA16, 0x1F, 19) | _carg(256, 0x3FF, 0),
        PhysicalAddr(tlut) & 0x1FFFFFF
    );
    rdpq_set_tile(RDPQ_TILE_INTERNAL, FMT_I4, TMEM_PALETTE_ADDR + color_idx*4*2, 256, NULL);
    rdpq_load_tlut_raw(RDPQ_TILE_INTERNAL, 0, num_colors);
}

static uint8_t size_inc[4] = {
    3,
    1,
    0,
    0,
};

static uint8_t size_shift[4] = {
    2,
    1,
    0,
    0,
};

#define G_TX_DTX_FRAC   11

int rdpq_load_block_size(const surface_t *tex) {
    int size = tex->flags & 0x3;
    return (tex->width * tex->height + size_inc[size]) >> size_shift[size];
}

int rdpq_load_block_tmem_pitch(const surface_t* tex) {
    int line_size = (TEX_FORMAT_BITDEPTH(tex->flags) * tex->width) >> 3;

    if (!line_size) {
        line_size = 1;
    }

    return line_size;
}

void rdpq_tex_upload_raw(rdpq_tile_t tile, const surface_t *tex, const rdpq_tileparms_t *parms, int tmem_addr) {
    rdpq_sync_tile();
    __rdpq_write8(RDPQ_CMD_SET_TEXTURE_IMAGE, _carg(FMT_RGBA16, 0x1F, 19) | _carg(tex->width-1, 0x3FF, 0), PhysicalAddr(tex->buffer) & 0x1FFFFFF);

    tex_format_t tex_fmt = surface_get_format(tex);
    tex_format_t load_fmt;
    
    if (tex_fmt == FMT_RGBA32) {
        load_fmt = FMT_RGBA32;
    } else {
        load_fmt = (tex_fmt & ~0x3) | FMT_RGBA16;
    }

    rdpq_set_tile(
        tile,
        load_fmt,
        tmem_addr,
        0,
        NULL
    );
    rdpq_sync_load();
    rdpq_load_block(tile, 0, 0, rdpq_load_block_size(tex), rdpq_load_block_tmem_pitch(tex));
    rdpq_sync_pipe();
}

struct rdpq_block_unsafe {
    struct rdpq_block_unsafe* next;
    uint32_t tracking;
    uint32_t cmds[] __attribute__((aligned(8)));  ///< RDP commands
};

struct rspq_block_unsafe {
    uint8_t nesting_level;   ///< Nesting level of the block
    uint8_t min_ph_level;    ///< Lowest slot used by any called placeholders, default: max nesting level
    uint8_t padding[2];      ///< Padding
    struct rdpq_block_unsafe *rdp_block; ///< Option RDP static buffer (with RDP commands)
    void *atexit; ///< List of callbacks to call upon freeing the block
    uint32_t cmds[];         ///< Block contents (commands)
};

uint32_t* rsqp_get_rdp_cmds(rspq_block_t* block) {
    struct rspq_block_unsafe* unsafe = (struct rspq_block_unsafe*)block;

    if (!unsafe->rdp_block) {
        return NULL;
    }

    return UncachedAddr(unsafe->rdp_block->cmds);
}