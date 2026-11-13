#include "resource_cache.h"

#include <libdragon.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "../config.h"

// a 32 bit prime number
#define MAGIC_PRIME 2748002342
#define NO_ENTRY    -1
#define MIN_TABLE_SIZE  32

#define DEBUG_LOADING_ORDER     0

#define HASH_RESOURCE(resource, mask)   ((((uint32_t)(resource) >> 4) * MAGIC_PRIME) & (mask))

#define MAX_RESOURCE_CACHES         8

// caches with entries waiting to be removed
static resource_cache_t* resource_caches_to_sweep[MAX_RESOURCE_CACHES];
static uint8_t resource_cache_sweep_count;
// false while sweeping since the RSP is known to be done with released resources
static bool resource_cache_needs_sync = true;
// incremented each time an entry is released
static uint16_t resource_cache_release_ticket;

void resource_cache_remove(struct resource_cache* cache, struct resource_cache_entry* entry, int entry_index, int resource_index);

// removes the entry from the cache and then calls the free callback on the resource
// the callback is called after removal since it may release resources in other caches
static void resource_cache_remove_and_free(struct resource_cache* cache, struct resource_cache_entry* entry, int entry_index, int resource_index) {
    void* resource = entry->resource;
    resource_cache_remove(cache, entry, entry_index, resource_index);

    if (cache->free_callback && resource) {
        cache->free_callback(resource);
    }
}

static inline bool resource_cache_ticket_reached(uint16_t entry_ticket, uint16_t sweep_ticket) {
    // handles the ticket wrapping around
    return (int16_t)(sweep_ticket - entry_ticket) >= 0;
}

// removes released entries up to and including sweep_ticket
// returns true if released entries still remain
static bool resource_cache_sweep_single(resource_cache_t* cache, uint16_t sweep_ticket) {
    bool has_remaining = false;

    for (int resource_index = 0; resource_index < cache->entry_capacity;) {
        int index = cache->resource_index[resource_index];

        if (index == NO_ENTRY) {
            resource_index += 1;
            continue;
        }

        resource_cache_entry_t* entry = &cache->entries[index];

        if (entry->reference_count > 0) {
            resource_index += 1;
            continue;
        }

        if (!resource_cache_ticket_reached(entry->release_ticket, sweep_ticket)) {
            // released after this sweep was scheduled so the RSP may still be using it
            has_remaining = true;
            resource_index += 1;
            continue;
        }

        // removing an entry can shift another entry into the
        // current slot so don't advance resource_index here
        resource_cache_remove_and_free(cache, entry, index, resource_index);
    }

    return has_remaining;
}

static void resource_cache_sweep_all(uint16_t sweep_ticket) {
    resource_cache_needs_sync = false;

    for (int cache_index = 0; cache_index < resource_cache_sweep_count;) {
        resource_cache_t* cache = resource_caches_to_sweep[cache_index];

        if (resource_cache_sweep_single(cache, sweep_ticket)) {
            cache_index += 1;
            continue;
        }

        cache->sweep_scheduled = false;
        resource_cache_sweep_count -= 1;
        resource_caches_to_sweep[cache_index] = resource_caches_to_sweep[resource_cache_sweep_count];
    }

    resource_cache_needs_sync = true;
}

static void resource_cache_sweep(void* data) {
    resource_cache_sweep_all((uint16_t)(uintptr_t)data);
}

static void resource_cache_schedule_sweep(resource_cache_t* cache, resource_cache_entry_t* entry) {
    resource_cache_release_ticket += 1;
    entry->release_ticket = resource_cache_release_ticket;

    // a sweep is scheduled for every release so entries are only removed
    // once the RSP has processed all commands queued before the release. 
    // Since sweeps remove all entries released up to their ticket, a sweep 
    // that is never called still gets handled by the next one
    rspq_call_deferred(resource_cache_sweep, (void*)(uintptr_t)resource_cache_release_ticket);

    if (cache->sweep_scheduled) {
        return;
    }

    assert(resource_cache_sweep_count < MAX_RESOURCE_CACHES);
    resource_caches_to_sweep[resource_cache_sweep_count] = cache;
    ++resource_cache_sweep_count;
    cache->sweep_scheduled = true;
}

void resource_cache_flush() {
    rspq_wait();
    // the RSP is idle so everything released can be removed
    resource_cache_sweep_all(resource_cache_release_ticket);
}

uint32_t resource_string_hash(const void* key) {
    uint32_t hash = 0;
    for (const unsigned char *p = key; *p != '\0'; p++) {
        hash = 31 * hash + *p;
    }
    return hash;
}


void resource_cache_destroy(struct resource_cache* cache) {
    if (!cache->entries) {
        return;
    }

    // entries with no references are waiting on a deferred sweep
    if (cache->sweep_scheduled) {
        resource_cache_flush();
    }
    assert(!cache->sweep_scheduled);

#if DEBUG_ENABLED
    for (int i = 0; i < cache->entry_count; i += 1) {
        debugf("resource still loaded %s\n", cache->entries[i].filename);
    }
#endif

    assert(!cache->entry_count);
    free(cache->entries);
    free(cache->filename_index);
    free(cache->resource_index);

    memset(cache, 0, sizeof(resource_cache_t));
}

void resource_cache_init(struct resource_cache* cache, resource_cache_free_callback free_callback) {
    cache->entry_capacity = MIN_TABLE_SIZE;
    cache->entry_count = 0;
    cache->free_callback = free_callback;

    cache->entries = malloc(sizeof(struct resource_cache_entry) * MIN_TABLE_SIZE);
    cache->filename_index = malloc(sizeof(short) * MIN_TABLE_SIZE);
    cache->resource_index = malloc(sizeof(short) * MIN_TABLE_SIZE);

    for (int i = 0; i < MIN_TABLE_SIZE; ++i) {
        cache->filename_index[i] = NO_ENTRY;
        cache->resource_index[i] = NO_ENTRY;
    }

    for (int i = 0; i < cache->entry_capacity; ++i) {
        struct resource_cache_entry* target_entry = &cache->entries[i];

        target_entry->filename = NULL;
        target_entry->resource = NULL;
        target_entry->filename_hash = 0;
        target_entry->filename_index = 0;
        target_entry->reference_count = 0;
    }
}

int resource_find_insert_index(short* index, int start_index, uint32_t mask) {
    while (index[start_index] != NO_ENTRY) {
        start_index = (start_index + 1) & mask;
    }
    return start_index;
}

int resource_find_entry(short* index, int start_index, uint32_t mask, int value) {
    while (index[start_index] != NO_ENTRY) {
        if (index[start_index] == value) {
            return start_index;
        }

        start_index = (start_index + 1) & mask;
    }
    return NO_ENTRY;

}

void resource_cache_resize(struct resource_cache* cache) {
    int new_capacity = cache->entry_capacity * 2;
    struct resource_cache_entry* new_entries = malloc(sizeof(struct resource_cache_entry) * new_capacity);
    short* new_filename_index = malloc((sizeof(short) * new_capacity));
    short* new_resource_index = malloc((sizeof(short) * new_capacity));

    for (int i = 0; i < new_capacity; ++i) {
        new_filename_index[i] = NO_ENTRY;
        new_resource_index[i] = NO_ENTRY;
    }

    memcpy(new_entries, cache->entries, sizeof(resource_cache_entry_t) * cache->entry_count);

    uint32_t new_mask = new_capacity - 1;

    for (int i = 0; i < cache->entry_count; ++i) {
        struct resource_cache_entry* target_entry = &new_entries[i];

        int filename_index = resource_find_insert_index(new_filename_index, target_entry->filename_hash & new_mask, new_mask);
        new_filename_index[filename_index] = i;
        target_entry->filename_index = filename_index;

        // entries without a resource yet get indexed in resource_cache_set_resource
        if (target_entry->resource) {
            new_resource_index[resource_find_insert_index(new_resource_index, HASH_RESOURCE(target_entry->resource, new_mask), new_mask)] = i;
        }
    }

    free(cache->entries);
    free(cache->filename_index);
    free(cache->resource_index);

    cache->entry_capacity = new_capacity;
    cache->entries = new_entries;
    cache->filename_index = new_filename_index;
    cache->resource_index = new_resource_index;
}

struct resource_cache_entry* resource_cache_insert(struct resource_cache* cache, int filename_index, uint32_t filename_hash, const char* filename) {
    uint32_t mask = cache->entry_capacity - 1;
    
    if (cache->entry_count + 1 > (cache->entry_capacity / 2)) {
        resource_cache_resize(cache);

        // recalculate filename_index posiiton
        mask = cache->entry_capacity - 1;
        filename_index = resource_find_insert_index(cache->filename_index, filename_hash & mask, mask);
    }

    int entry_index = cache->entry_count;
    
    struct resource_cache_entry* entry = &cache->entries[entry_index];

    int filename_len = strlen(filename);
    entry->filename = malloc(filename_len  + 1);
    strcpy(entry->filename, filename);
    entry->resource = NULL;
    entry->reference_count = 1;
    entry->filename_index = filename_index;
    entry->filename_hash = filename_hash;

    cache->filename_index[filename_index] = entry_index;

    cache->entry_count += 1;

    return entry;
}

struct resource_cache_entry* resource_cache_use(struct resource_cache* cache, const char* filename) {
#if DEBUG_LOADING_ORDER
    debugf("resource_cache_use filename = %s\n", filename);
#endif
    assert(cache->entries);

    uint32_t filename_hash = resource_string_hash(filename);

    uint32_t mask = cache->entry_capacity - 1;
    uint32_t index_check = filename_hash & mask;

    for (;;) {
        int index = cache->filename_index[index_check];

        if (index == NO_ENTRY) {
            return resource_cache_insert(cache, index_check, filename_hash, filename);
        }

        struct resource_cache_entry* entry = &cache->entries[index];

        if (entry->filename_hash == filename_hash && strcmp(filename, entry->filename) == 0) {
            entry->reference_count += 1;
            return entry;
        }

        index_check = (index_check + 1) & mask;
    }
}

// returns true if the slot at index is not reachable from home 
// without passing through the empty slot at empty_index
static inline bool resource_cache_should_shift(int home, int empty_index, int index) {
    if (empty_index <= index) {
        return home <= empty_index || home > index;
    } else {
        return home <= empty_index && home > index;
    }
}

// backward shift deletion for linear probing. Entries are only ever moved
// backwards towards the removed slot, never past their home slot
void resource_cache_adjust_resource_index(struct resource_cache* cache, int resource_index, uint32_t mask) {
    int empty_index = resource_index;
    int index = (resource_index + 1) & mask;

    while (cache->resource_index[index] != NO_ENTRY) {
        int entry_index = cache->resource_index[index];
        struct resource_cache_entry* entry = &cache->entries[entry_index];
        int home = HASH_RESOURCE(entry->resource, mask);

        if (resource_cache_should_shift(home, empty_index, index)) {
            cache->resource_index[empty_index] = entry_index;
            cache->resource_index[index] = NO_ENTRY;
            empty_index = index;
        }

        index = (index + 1) & mask;
    }
}

void resource_cache_adjust_filename_index(struct resource_cache* cache, int filename_index, uint32_t mask) {
    int empty_index = filename_index;
    int index = (filename_index + 1) & mask;

    while (cache->filename_index[index] != NO_ENTRY) {
        int entry_index = cache->filename_index[index];
        struct resource_cache_entry* entry = &cache->entries[entry_index];
        int home = entry->filename_hash & mask;

        if (resource_cache_should_shift(home, empty_index, index)) {
            cache->filename_index[empty_index] = entry_index;
            entry->filename_index = empty_index;
            cache->filename_index[index] = NO_ENTRY;
            empty_index = index;
        }

        index = (index + 1) & mask;
    }
}

void resource_cache_remove(struct resource_cache* cache, struct resource_cache_entry* entry, int entry_index, int resource_index) {
    int filename_index = entry->filename_index;

    free(entry->filename);

    cache->entry_count -= 1;
    uint32_t mask = cache->entry_capacity - 1;

    if (entry_index != cache->entry_count) {
        int last_entry_index = cache->entry_count;
        struct resource_cache_entry* last_entry = &cache->entries[last_entry_index];

        *entry = *last_entry;

        cache->filename_index[entry->filename_index] = entry_index;
    }

    // the entry that was moved may not have a resource assigned yet
    if (entry_index != cache->entry_count && entry->resource) {
        int last_entry_index = cache->entry_count;
        int existing_resource_index = resource_find_entry(cache->resource_index, HASH_RESOURCE(entry->resource, mask), mask, last_entry_index);

#if DEBUG_ENABLED
        if (existing_resource_index == NO_ENTRY) {
            debugf("something was wrong! %d %d %d\n", last_entry_index, entry_index, existing_resource_index);
        }
#endif

        assert(existing_resource_index != NO_ENTRY);
        cache->resource_index[existing_resource_index] = entry_index;
    }

    cache->filename_index[filename_index] = NO_ENTRY;
    cache->resource_index[resource_index] = NO_ENTRY;
    resource_cache_adjust_resource_index(cache, resource_index, mask);
    resource_cache_adjust_filename_index(cache, filename_index, mask);
}

void resource_cache_free(struct resource_cache* cache, void* resource) {
#if DEBUG_LOADING_ORDER
    debugf("resource_cache_free resource = %08x\n", resource);
#endif
    assert(resource);
    assert(cache->entries);

    uint32_t mask = cache->entry_capacity - 1;
    uint32_t index_check = HASH_RESOURCE(resource, mask);

    for (;;) {
        int index = cache->resource_index[index_check];

#if DEBUG_ENABLED
        if (index == NO_ENTRY) {
            int entry_index = 0;
            
            for (; entry_index < cache->entry_count; entry_index += 1) {
                struct resource_cache_entry* entry = &cache->entries[entry_index];
                
                if (entry->resource == resource) {
                    break;
                }
            }

            int resource_index = 0;

            for (; resource_index < cache->entry_capacity; resource_index += 1) {
                if (cache->resource_index[resource_index] == entry_index) {
                    break;
                }
            }

            debugf("something was wrong! %d %d %d %d\n", (int)index_check, cache->entry_capacity, entry_index, resource_index);
        }
#endif

        assert(index != NO_ENTRY);
        
        struct resource_cache_entry* entry = &cache->entries[index];

        if (entry->resource == resource) {
            entry->reference_count -= 1;

            if (entry->reference_count == 0) {
                if (resource_cache_needs_sync) {
                    resource_cache_schedule_sweep(cache, entry);
                } else {
                    resource_cache_remove_and_free(cache, entry, index, index_check);
                }
            }

            return;
        }

        index_check = (index_check + 1) & mask;
    }
}

void resource_cache_set_resource(struct resource_cache* cache, struct resource_cache_entry* entry, void* resource) {
#if DEBUG_LOADING_ORDER
    debugf("resource_cache_set_resource filename = %s resource = %08x\n", entry->filename, (int)resource);
#endif
    assert(!entry->resource);

    entry->resource = resource;

    uint32_t mask = cache->entry_capacity - 1;
    
    uint32_t index_check = resource_find_insert_index(cache->resource_index, HASH_RESOURCE(resource, mask), mask);
    cache->resource_index[index_check] = entry - cache->entries;
}