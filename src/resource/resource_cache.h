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
// swaps the contents of new_resource into existing_resource in place so
// existing pointers to existing_resource pick up the change. Afterwards
// new_resource is passed to the free callback so it must be left holding
// the old contents, or left untouched if the swap is rejected
typedef void (*resource_cache_swap_resource)(void* new_resource, void* existing_resource);
typedef void* (*resource_cache_load)(const char* filename);

struct resource_cache {
    struct resource_cache_entry* entries;
    short entry_capacity;
    short entry_count;

    short* filename_index;
    short* resource_index;

    resource_cache_load load;
    resource_cache_free_callback free_callback;
    resource_cache_swap_resource swap;
    bool sweep_scheduled;
};

typedef struct resource_cache resource_cache_t;

// sets the callbacks for a cache. If swap and ext are both set, files ending
// in ext are hot reloaded in place. Registering the same cache again replaces
// the previous registration. Entries are allocated on first use
void resource_cache_register(struct resource_cache* cache, resource_cache_load load, resource_cache_free_callback free_callback, resource_cache_swap_resource swap, const char* ext);
// reloads filename into the existing resource if it is currently loaded
// returns true if a resource was reloaded
bool resource_cache_reload(const char* filename);
bool resource_cache_reload_single(struct resource_cache* cache, const char* filename);

void resource_cache_init_entries(struct resource_cache* cache);
resource_cache_entry_t* resource_cache_find_entry(struct resource_cache* cache, const char* filename, bool create_on_empty);

// processes any pending deferred removals and then frees the cache
// all resources must have been released before calling this
void resource_cache_destroy(struct resource_cache* cache);
// returns the resource for filename, loading it if needed, 
// and increments its reference count
void* resource_cache_use(struct resource_cache* cache, const char* filename);
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