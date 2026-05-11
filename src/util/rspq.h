#ifndef __UTIL_RSPQ_H__
#define __UTIL_RSPQ_H__

#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>

uint32_t rspq_count_overlay_switches(rspq_block_t* block, uint32_t* ending_overlay);

#endif