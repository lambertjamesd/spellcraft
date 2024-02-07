#include "resource_cache.h"

#include <malloc.h>
#include <string.h>

// a 32 bit prime number
#define MAGIC_PRIME 2748002342
#define NO_ENTRY    -1
#define MIN_TABLE_SIZE  32

uint32_t string_hash(const void* key) {
    uint32_t hash = 0;
    for (const unsigned char *p = key; *p != '\0'; p++) {
        hash = 31 * hash + *p;
    }
    return hash;
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

    cache->next_entry_index = 0;

    int new_mask = new_capacity - 1;

    for (int i = 0; i < cache->entry_capacity; ++i) {
        struct resource_cache_entry* entry = &cache->entries[i];

        // skip empty entries
        if (!entry->filename) {
            continue;
        }

        struct resource_cache_entry* target_entry = &new_entries[cache->next_entry_index];

        target_entry->filename = entry->filename;
        target_entry->resource = entry->resource;
        target_entry->reference_count = entry->reference_count;
        target_entry->filename_hash = entry->filename_hash;

        // insert filename index
        uint32_t index_check = target_entry->filename_hash & new_mask;
        while (new_filename_index[index_check] != NO_ENTRY) {
            index_check = (index_check + 1) & new_mask;
        }

        new_filename_index[index_check] = cache->next_entry_index;
        target_entry->filename_index = index_check;

        // insert resource index
        index_check = ((uint32_t)entry->resource * MAGIC_PRIME) & new_mask;

        while (new_resource_index[index_check] != NO_ENTRY) {
            index_check = (index_check + 1) & new_mask;
        }

        new_resource_index[index_check] = cache->next_entry_index;

        // move to next entry index
        cache->next_entry_index += 1;
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
        filename_index = filename_hash & mask;
        while (cache->filename_index[filename_index] != NO_ENTRY) {
            filename_index = (filename_index + 1) & mask;
        }
    }

    int entry_index = cache->next_entry_index;

    // search for an available entry
    while (cache->entries[entry_index].filename) {
        entry_index = (entry_index + 1) & mask;
    }

    struct resource_cache_entry* entry = &cache->entries[entry_index];

    int filename_len = strlen(filename);
    entry->filename = malloc(filename_len  + 1);
    strcpy(entry->filename, filename);
    entry->resource = NULL;
    entry->reference_count = 1;
    entry->filename_index = filename_index;
    entry->filename_hash = filename_hash;

    cache->entry_count += 1;

    return entry;
}

struct resource_cache_entry* resource_cache_use(struct resource_cache* cache, const char* filename) {
    uint32_t filename_hash = string_hash(filename);

    uint32_t mask = cache->entry_capacity - 1;
    uint32_t index_check = filename_hash & mask;

    for (;;) {
        int index = cache->filename_index[index_check];

        if (index == NO_ENTRY) {
            return resource_cache_insert(cache, index, filename_hash, filename);
        }

        struct resource_cache_entry* entry = &cache->entries[index];

        if (entry->filename_hash == filename_hash && strcmp(filename, entry->filename) == 0) {
            entry->reference_count += 1;
            return entry;
        }

        index_check = (index_check + 1) & mask;
    }
}

void resource_cache_remove(struct resource_cache* cache, struct resource_cache_entry* entry, int resource_index) {
    int filename_index = entry->filename_index;

    cache->filename_index[filename_index] = NO_ENTRY;
    cache->resource_index[resource_index] = NO_ENTRY;
    free(entry->filename);
    entry->filename = NULL;
    entry->reference_count = 0;
    entry->resource = NULL;
    entry->filename_index = 0;

    cache->entry_count -= 1;
}

bool resource_cache_free(struct resource_cache* cache, void* resource) {
    uint32_t mask = cache->entry_capacity - 1;
    uint32_t index_check = ((uint32_t)resource * MAGIC_PRIME) & mask;

    for (;;) {
        int index = cache->resource_index[index_check];

        if (index == NO_ENTRY) {
            // resource not found
            return false;
        }

        struct resource_cache_entry* entry = &cache->entries[index];

        if (entry->resource == resource) {
            entry->reference_count -= 1;

            if (entry->reference_count == 0) {
                resource_cache_remove(cache, entry, index_check);
                return true;
            }

            return false;
        }

        index_check = (index_check + 1) & mask;
    }

    return false;
}