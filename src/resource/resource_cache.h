#ifndef __RESOURCE_RESOURCE_CACHE_H__
#define __RESOURCE_RESOURCE_CACHE_H__

#include <stdbool.h>
#include <stdint.h>

struct resource_cache_entry {
    char* filename;
    void* resource;
    short reference_count;
    short filename_index;
    uint32_t filename_hash;
};

typedef struct resource_cache_entry resource_cache_entry_t;

struct resource_cache {
    struct resource_cache_entry* entries;
    short entry_capacity;
    short entry_count;

    short* filename_index;
    short* resource_index;
};

typedef struct resource_cache resource_cache_t;

void resource_cache_init(struct resource_cache* cache);
void resource_cache_destroy(struct resource_cache* cache);
struct resource_cache_entry* resource_cache_use(struct resource_cache* cache, const char* filename);
bool resource_cache_free(struct resource_cache* cache, void* resource);

void resource_cache_set_resource(struct resource_cache* cache, struct resource_cache_entry* entry, void* resource);

#endif