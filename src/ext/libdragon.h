#ifndef __EXT_LIBDRAGON_H__
#define __EXT_LIBDRAGON_H__

#include <stdint.h>
#include <libdragon.h>

void rdpq_write_other_modes_raw(uint32_t w0, uint32_t w1);
void rdpq_tex_upload_tlut_raw(uint16_t *tlut, int color_idx, int num_colors);
void rdpq_tex_upload_raw(rdpq_tile_t tile, const surface_t *tex, const rdpq_tileparms_t *parms, int tmem_addr);

#endif