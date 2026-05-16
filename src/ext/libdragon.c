#include "libdragon.h"

#include <libdragon.h>

void __rdpq_write8(uint32_t cmd_id, uint32_t arg0, uint32_t arg1);

void rdpq_write_other_modes_raw(uint32_t w0, uint32_t w1) {
    __rdpq_write8(RDPQ_CMD_SET_OTHER_MODES, w0, w1);
}