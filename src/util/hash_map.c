#include "hash_map.h"

#include <malloc.h>
#include <memory.h>

// 2^32 / golden ratio, odd so multiplication is a bijection on uint32
#define HASH_MULTIPLIER 0x9E3779B1u
#define MIN_CAPACITY    8
#define MAX_CAPACITY    32768

// key 0 marks an empty slot
#define EMPTY_KEY       0

static inline uint32_t hash_map_hash(int key) {
    uint32_t h = (uint32_t)key * HASH_MULTIPLIER;
    // the low bits of a multiply only depend on the low bits of the key
    // fold the well mixed high bits down so masking uses them
    return h ^ (h >> 16);
}

// max load factor of 3/4
static inline bool hash_map_needs_grow(int count, int capacity) {
    return count * 4 >= capacity * 3;
}

static int hash_map_capacity_for(int count) {
    int capacity = MIN_CAPACITY;

    while (capacity < MAX_CAPACITY && hash_map_needs_grow(count, capacity)) {
        capacity <<= 1;
    }

    return capacity;
}

static bool hash_map_alloc(struct hash_map* hash_map, int capacity) {
    struct hash_map_entry* entries = malloc(sizeof(struct hash_map_entry) * capacity);

    if (!entries) {
        return false;
    }

    memset(entries, 0, sizeof(struct hash_map_entry) * capacity);

    hash_map->entries = entries;
    hash_map->capacity = capacity;

    return true;
}

bool hash_map_init(struct hash_map* hash_map, int capacity) {
    hash_map->count = 0;

    if (!hash_map_alloc(hash_map, hash_map_capacity_for(capacity))) {
        hash_map->entries = NULL;
        hash_map->capacity = 0;
        return false;
    }

    return true;
}

void hash_map_destroy(struct hash_map* hash_map) {
    free(hash_map->entries);
    hash_map->entries = NULL;
    hash_map->capacity = 0;
    hash_map->count = 0;
}

// returns the slot holding key or the empty slot where it would be inserted
// the load factor guarantees an empty slot exists so the probe terminates
static inline struct hash_map_entry* hash_map_find_entry(struct hash_map_entry* entries, int capacity, int key) {
    uint32_t mask = capacity - 1;
    uint32_t index = hash_map_hash(key) & mask;

    for (;;) {
        struct hash_map_entry* entry = &entries[index];

        if (entry->key == key || entry->key == EMPTY_KEY) {
            return entry;
        }

        index = (index + 1) & mask;
    }
}

static bool hash_map_resize(struct hash_map* hash_map) {
    if (hash_map->capacity >= MAX_CAPACITY) {
        return false;
    }

    struct hash_map_entry* old_entries = hash_map->entries;
    int old_capacity = hash_map->capacity;

    if (!hash_map_alloc(hash_map, old_capacity * 2)) {
        return false;
    }

    for (int i = 0; i < old_capacity; i += 1) {
        struct hash_map_entry* prev_entry = &old_entries[i];

        if (prev_entry->key == EMPTY_KEY) {
            continue;
        }

        *hash_map_find_entry(hash_map->entries, hash_map->capacity, prev_entry->key) = *prev_entry;
    }

    free(old_entries);

    return true;
}

void* hash_map_get(struct hash_map* hash_map, int key) {
    if (!hash_map->capacity || key == EMPTY_KEY) {
        return NULL;
    }

    // an empty slot has a NULL value
    return hash_map_find_entry(hash_map->entries, hash_map->capacity, key)->value;
}

bool hash_map_set(struct hash_map* hash_map, int key, void* value) {
    if (key == EMPTY_KEY) {
        return false;
    }

    if (!hash_map->capacity && !hash_map_init(hash_map, MIN_CAPACITY)) {
        return false;
    }

    struct hash_map_entry* result = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);

    if (result->key == key) {
        result->value = value;
        return true;
    }

    if (hash_map_needs_grow(hash_map->count + 1, hash_map->capacity)) {
        if (!hash_map_resize(hash_map)) {
            return false;
        }

        result = hash_map_find_entry(hash_map->entries, hash_map->capacity, key);
    }

    result->key = key;
    result->value = value;
    hash_map->count += 1;

    return true;
}

void hash_map_delete(struct hash_map* hash_map, int key) {
    if (!hash_map->capacity || key == EMPTY_KEY) {
        return;
    }

    struct hash_map_entry* entries = hash_map->entries;
    uint32_t mask = hash_map->capacity - 1;
    struct hash_map_entry* entry = hash_map_find_entry(entries, hash_map->capacity, key);

    if (entry->key != key) {
        return;
    }

    hash_map->count -= 1;

    // backward shift deletion, no tombstones needed
    uint32_t hole = entry - entries;
    uint32_t next = (hole + 1) & mask;

    while (entries[next].key != EMPTY_KEY) {
        uint32_t home = hash_map_hash(entries[next].key) & mask;

        // the entry can fill the hole if its home is not
        // cyclically within (hole, next]
        if (((next - home) & mask) >= ((next - hole) & mask)) {
            entries[hole] = entries[next];
            hole = next;
        }

        next = (next + 1) & mask;
    }

    entries[hole].key = EMPTY_KEY;
    entries[hole].value = NULL;
}

struct hash_map_entry* hash_map_next(struct hash_map* hash_map, struct hash_map_entry* curr) {
    if (!hash_map->entries || !hash_map->count) {
        return NULL;
    }

    struct hash_map_entry* end = hash_map->entries + hash_map->capacity;
    curr = curr ? curr + 1 : hash_map->entries;

    for (; curr < end; ++curr) {
        if (curr->key != EMPTY_KEY) {
            return curr;
        }
    }

    return NULL;
}
