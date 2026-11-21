#ifndef __UTIL_HASHMAP_H__
#define __UTIL_HASHMAP_H__

#include <stdint.h>
#include <stdbool.h>

struct hash_map_entry {
    int key;
    void* value;
};

struct hash_map {
    struct hash_map_entry* entries;
    uint16_t capacity;
    uint16_t count;
};

typedef struct hash_map hash_map_t;

// open addressing with linear probing and a max load factor of 3/4
// key 0 is reserved to mark empty slots and cannot be stored
// NULL values may be stored but hash_map_get can't distinguish them from missing keys

// capacity is the number of entries to hold before growing
bool hash_map_init(struct hash_map* hash_map, int capacity);
void hash_map_destroy(struct hash_map* hash_map);

void* hash_map_get(struct hash_map* hash_map, int key);
// returns false if key is 0 or memory could not be allocated
bool hash_map_set(struct hash_map* hash_map, int key, void* value);
void hash_map_delete(struct hash_map* hash_map, int key);

// iterates over all entries, pass NULL to get the first entry
// do not set or delete while iterating, entries may be moved
struct hash_map_entry* hash_map_next(struct hash_map* hash_map, struct hash_map_entry* curr);

static inline bool hash_map_is_ready(hash_map_t* hash_map) {
    return hash_map->capacity > 0;
}

#endif