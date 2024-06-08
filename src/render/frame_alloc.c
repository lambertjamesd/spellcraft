
#include "frame_alloc.h"
#include <stddef.h>

void frame_pool_reset(struct frame_memory_pool* pool) {
    pool->current_word = 0;
}

void* frame_malloc(struct frame_memory_pool* pool, int bytes) {
    if (pool->current_word >= FRAME_WORD_COUNT) {
        return NULL;
    }

    void* result = &pool->memory[pool->current_word];
    pool->current_word += (bytes + 7) >> 3;
    return result;
}