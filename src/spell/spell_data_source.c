#include "spell_data_source.h"


void spell_data_source_pool_init(struct spell_data_source_pool* pool) {
    for (int i = 0; i < MAX_SPELL_DATA_SOURCES; ++i) {
        pool->data_sources[i].reference_count = 0;
    }

    pool->next_data_source = 0;
}

struct spell_data_source* spell_data_source_pool_get(struct spell_data_source_pool* pool) {
    for (int i = 0; i < MAX_SPELL_DATA_SOURCES; ++i) {
        struct spell_data_source* current = &pool->data_sources[pool->next_data_source];

        pool->next_data_source += 1;

        if (pool->next_data_source == MAX_SPELL_DATA_SOURCES) {
            pool->next_data_source = 0;
        }

        if (current->reference_count == 0) {
            return current;
        }
    }

    return 0;
}