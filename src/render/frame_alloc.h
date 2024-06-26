#ifndef __RENDER_FRAME_ALLOC_H__
#define __RENDER_FRAME_ALLOC_H__

#include <stdint.h>

#define FRAME_MEMORY_SIZE   32 * 1024
#define FRAME_WORD_COUNT    (FRAME_MEMORY_SIZE / sizeof(uint64_t))

struct frame_memory_pool {
    uint64_t memory[FRAME_WORD_COUNT];
    uint16_t current_word;
} __attribute__((aligned(16)));

void frame_pool_reset(struct frame_memory_pool* pool);
void* frame_malloc(struct frame_memory_pool* pool, int bytes);

#endif