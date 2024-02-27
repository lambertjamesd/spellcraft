#ifndef __SPELL_SPELL_DATA_SOURCE_H__
#define __SPELL_SPELL_DATA_SOURCE_H__

#include "../math/vector3.h"

#include <stdint.h>

#define MAX_SPELL_DATA_SOURCES  32

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

struct spell_data_source_pool {
    struct spell_data_source data_sources[MAX_SPELL_DATA_SOURCES];
    uint16_t next_data_source;
};

void spell_data_source_pool_init(struct spell_data_source_pool* pool);
struct spell_data_source* spell_data_source_pool_get(struct spell_data_source_pool* pool);

#endif