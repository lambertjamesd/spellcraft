#ifndef __SPELL_SPELL_DATA_SOURCE_H__
#define __SPELL_SPELL_DATA_SOURCE_H__

#include "../math/vector3.h"

#include <stdint.h>

enum spell_source_flags {
    SPELL_SOURCE_FLAMING = (1 << 0),
    SPELL_SOURCE_CONTROLLED = (1 << 1),
    SPELL_SOURCE_ACTIVE = (1 << 2),
};

struct spell_data_source {
    struct Vector3 position;
    struct Vector3 direction;

    uint16_t flags;
    uint8_t reference_count;
};

#endif