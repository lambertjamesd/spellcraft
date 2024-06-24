
#include <libdragon.h>
#include <stddef.h>
#include "frame_alloc.h"

void frame_pool_reset(struct frame_memory_pool* pool) {
    pool->current_word = 0;
}

void frame_pool_finish(struct frame_memory_pool* pool) {
    data_cache_hit_writeback_invalidate(&pool->memory[0], pool->current_word * sizeof(uint64_t));
}

void* frame_malloc(struct frame_memory_pool* pool, int bytes) {
    if (pool->current_word >= FRAME_WORD_COUNT) {
        return NULL;
    }

    void* result = &pool->memory[pool->current_word];
    pool->current_word += (bytes + 7) >> 3;

    // align to 16 bytes
    if (pool->current_word & 1) {
        pool->current_word += 1;
    }

    return result;
}