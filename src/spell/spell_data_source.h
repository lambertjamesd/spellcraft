#ifndef __SPELL_SPELL_DATA_SOURCE_H__
#define __SPELL_SPELL_DATA_SOURCE_H__

#include "../math/vector3.h"

#include <stdint.h>

union spell_source_flags {
    struct {
        uint16_t flaming: 1;
        uint16_t controlled: 1;
        uint16_t active: 1;
    };
    uint16_t all;
};

struct spell_data_source {
    struct Vector3 position;
    struct Vector3 direction;

    union spell_source_flags flags;
    uint8_t reference_count;
};

#endif