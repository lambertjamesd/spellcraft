#ifndef __UTIL_HASHMAP_H__
#define __UTIL_HASHMAP_H__

#include <stdint.h>

struct hash_map_entry {
    int key;
    void* value;
};

struct hash_map {
    struct hash_map_entry* entries;
    uint16_t capacity;
    uint16_t count;
};

// capacity must be a power of 2
void hash_map_init(struct hash_map* hash_map, int capacity);
void hash_map_destroy(struct hash_map* hash_map);

void* hash_map_get(struct hash_map* hash_map, int key);
void hash_map_set(struct hash_map* hash_map, int key, void* value);
void hash_map_delete(struct hash_map* hash_map, int key);

#endif