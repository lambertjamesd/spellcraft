#include "staff.h"

#include "../scene/scene_definition.h"

struct staff_stats staff_stats[INV_STAFF_COUNT] = {
    {ITEM_TYPE_STAFF_DEFAULT, 0},
};

struct staff_stats staff_stats_none = {
    ITEM_TYPE_NONE, 0xFF,
};