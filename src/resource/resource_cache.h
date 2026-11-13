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
    // used to know when it is safe to remove an entry once released
    uint16_t release_ticket;
};

typedef struct resource_cache_entry resource_cache_entry_t;

typedef void (*resource_cache_free_callback)(void* resource);

struct resource_cache {
    struct resource_cache_entry* entries;
    short entry_capacity;
    short entry_count;

    short* filename_index;
    short* resource_index;

    resource_cache_free_callback free_callback;
    bool sweep_scheduled;
};

typedef struct resource_cache resource_cache_t;

void resource_cache_init(struct resource_cache* cache, resource_cache_free_callback free_callback);

static inline void resource_cache_maybe_init(struct resource_cache* cache, resource_cache_free_callback free_callback) {
    if (cache->entries) {
        return;
    }
    resource_cache_init(cache, free_callback);
}

// processes any pending deferred removals and then frees the cache
// all resources must have been released before calling this
void resource_cache_destroy(struct resource_cache* cache);
struct resource_cache_entry* resource_cache_use(struct resource_cache* cache, const char* filename);
// decrements the reference count of resource. Once it reaches 0 the resource
// is removed from the cache and free_callback is called on it. Unless called
// from within a deferred sweep, the removal is deferred until a sweep queued
// by resource_cache_update has been reached by the RSP, so the resource may 
// still be revived by resource_cache_use until then
void resource_cache_free(struct resource_cache* cache, void* resource);

// queues a deferred sweep for released resources if one isn't already
// pending. Call once per frame from the main loop, outside of any rspq block
// or deferred call
void resource_cache_update();

// waits for the RSP to finish and then removes all released resources
void resource_cache_flush();

void resource_cache_set_resource(struct resource_cache* cache, struct resource_cache_entry* entry, void* resource);

#endif