
#include <libdragon.h>
#include <stddef.h>
#include "frame_alloc.h"

static struct frame_memory_pool frame_memory_pools[2];
static uint8_t next_frame_memory_pool;

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

T3DMat4FP* frame_pool_get_transformfp(struct frame_memory_pool* pool) {
    return UncachedAddr(frame_malloc(pool, sizeof(T3DMat4FP)));
}

frame_memory_pool_t* frame_pool_curr() {
    return &frame_memory_pools[next_frame_memory_pool];
}

void frame_pool_reset() {
    frame_pool_curr()->current_word = 0;
}

void frame_pool_next() {
            next_frame_memory_pool ^= 1;

}