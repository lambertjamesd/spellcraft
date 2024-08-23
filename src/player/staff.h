#ifndef __PLAYER_STAFF_H__
#define __PLAYER_STAFF_H__

#include <stdint.h>

#define INV_STAFF_COUNT 4

struct staff_stats {
    uint8_t item_type;
    uint8_t staff_index;
};

extern struct staff_stats staff_stats[INV_STAFF_COUNT];
extern struct staff_stats staff_stats_none;

#endif