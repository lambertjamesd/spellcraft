#include "hash_map.h"

#include "../test/framework_test.h"

#include <stddef.h>
#include <stdint.h>

#define TEST_VALUE(x)   ((void*)(intptr_t)(x))
#define TEST_INT(x)     ((int)(intptr_t)(x))

static int test_hash_map_slot(struct hash_map* map, int key) {
    for (int i = 0; i < map->capacity; i += 1) {
        if (map->entries[i].key == key) {
            return i;
        }
    }

    return -1;
}

// the slot a key lands in when inserted into an empty map
static int test_hash_map_home(int capacity, int key) {
    hash_map_t probe;
    hash_map_init(&probe, capacity);
    hash_map_set(&probe, key, TEST_VALUE(1));
    int result = test_hash_map_slot(&probe, key);
    hash_map_destroy(&probe);
    return result;
}

// finds a key starting from start that has the given home slot
static int test_hash_map_key_with_home(int capacity, int home, int start) {
    for (int candidate = start; candidate < 100000; candidate += 1) {
        if (test_hash_map_home(capacity, candidate) == home) {
            return candidate;
        }
    }

    return 0;
}

static void test_hash_map_basic(struct test_context* t) {
    hash_map_t map;

    test_eqi(t, true, hash_map_init(&map, 8));
    test_eqi(t, true, hash_map_is_ready(&map));
    test_eqi(t, 0, map.count);

    test_eqi(t, 0, TEST_INT(hash_map_get(&map, 1)));
    test_eqi(t, true, hash_map_set(&map, 1, TEST_VALUE(1)));
    test_eqi(t, 1, TEST_INT(hash_map_get(&map, 1)));
    test_eqi(t, 1, map.count);

    // overwriting a key doesn't change the count
    test_eqi(t, true, hash_map_set(&map, 1, TEST_VALUE(2)));
    test_eqi(t, 2, TEST_INT(hash_map_get(&map, 1)));
    test_eqi(t, 1, map.count);

    // negative keys
    test_eqi(t, true, hash_map_set(&map, -5, TEST_VALUE(5)));
    test_eqi(t, 5, TEST_INT(hash_map_get(&map, -5)));
    test_eqi(t, 2, map.count);

    // key 0 is reserved
    test_eqi(t, false, hash_map_set(&map, 0, TEST_VALUE(3)));
    test_eqi(t, 0, TEST_INT(hash_map_get(&map, 0)));
    test_eqi(t, 2, map.count);

    // deleting a missing key does nothing
    hash_map_delete(&map, 1234);
    hash_map_delete(&map, 0);
    test_eqi(t, 2, map.count);

    hash_map_delete(&map, 1);
    test_eqi(t, 0, TEST_INT(hash_map_get(&map, 1)));
    test_eqi(t, 1, map.count);

    // deleting twice doesn't decrement count twice
    hash_map_delete(&map, 1);
    test_eqi(t, 1, map.count);

    hash_map_destroy(&map);
    test_eqi(t, false, hash_map_is_ready(&map));
}

static void test_hash_map_init_capacity(struct test_context* t) {
    int requested[] = {0, 1, 3, 8, 10, 33, 100};

    for (int i = 0; i < (int)(sizeof(requested) / sizeof(*requested)); i += 1) {
        hash_map_t map;
        hash_map_init(&map, requested[i]);

        // capacity must be a power of 2 for masking to work
        test_eqi(t, 0, map.capacity & (map.capacity - 1));
        test_eqi(t, true, map.capacity >= 8);

        // the requested count should fit without growing
        struct hash_map_entry* entries = map.entries;
        for (int key = 1; key <= requested[i]; key += 1) {
            hash_map_set(&map, key, TEST_VALUE(key));
        }
        test_eqi(t, (int)(intptr_t)entries, (int)(intptr_t)map.entries);

        for (int key = 1; key <= requested[i]; key += 1) {
            test_eqi(t, key, TEST_INT(hash_map_get(&map, key)));
        }

        hash_map_destroy(&map);
    }
}

static void test_hash_map_collision(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);

    int a = 1;
    int b = test_hash_map_key_with_home(8, test_hash_map_home(8, a), a + 1);
    int c = test_hash_map_key_with_home(8, test_hash_map_home(8, a), b + 1);
    test_neqi(t, 0, b);
    test_neqi(t, 0, c);

    hash_map_set(&map, a, TEST_VALUE(10));
    hash_map_set(&map, b, TEST_VALUE(20));
    hash_map_set(&map, c, TEST_VALUE(30));

    int home = test_hash_map_slot(&map, a);
    int mask = map.capacity - 1;
    test_eqi(t, (home + 1) & mask, test_hash_map_slot(&map, b));
    test_eqi(t, (home + 2) & mask, test_hash_map_slot(&map, c));

    // delete entry where collided entries want to be
    hash_map_delete(&map, a);

    test_eqi(t, 0, TEST_INT(hash_map_get(&map, a)));
    test_eqi(t, 20, TEST_INT(hash_map_get(&map, b)));
    test_eqi(t, 30, TEST_INT(hash_map_get(&map, c)));

    // entries should be shifted back toward their home slot
    test_eqi(t, home, test_hash_map_slot(&map, b));
    test_eqi(t, (home + 1) & mask, test_hash_map_slot(&map, c));
    test_eqi(t, 0, map.entries[(home + 2) & mask].key);

    hash_map_destroy(&map);
}

// deleting must not move an entry before its home slot
static void test_hash_map_delete_keeps_home(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);
    int mask = map.capacity - 1;

    int a = 1;
    int a_home = test_hash_map_home(8, a);
    int b = test_hash_map_key_with_home(8, (a_home + 1) & mask, a + 1);

    hash_map_set(&map, a, TEST_VALUE(1));
    hash_map_set(&map, b, TEST_VALUE(2));
    test_neqi(t, 0, b);

    hash_map_delete(&map, a);

    // b is already in its home slot and must stay there
    test_eqi(t, (a_home + 1) & mask, test_hash_map_slot(&map, b));
    test_eqi(t, 2, TEST_INT(hash_map_get(&map, b)));

    hash_map_destroy(&map);
}

// a cluster that wraps from the end of the table to the start
static void test_hash_map_wrap_around(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);
    int last = map.capacity - 1;

    int keys[3];
    keys[0] = test_hash_map_key_with_home(8, last, 1);
    keys[1] = test_hash_map_key_with_home(8, last, keys[0] + 1);
    keys[2] = test_hash_map_key_with_home(8, last, keys[1] + 1);
    test_neqi(t, 0, keys[2]);

    for (int i = 0; i < 3; i += 1) {
        hash_map_set(&map, keys[i], TEST_VALUE(i + 1));
    }

    test_eqi(t, last, test_hash_map_slot(&map, keys[0]));
    test_eqi(t, 0, test_hash_map_slot(&map, keys[1]));
    test_eqi(t, 1, test_hash_map_slot(&map, keys[2]));

    hash_map_delete(&map, keys[0]);

    test_eqi(t, last, test_hash_map_slot(&map, keys[1]));
    test_eqi(t, 0, test_hash_map_slot(&map, keys[2]));
    test_eqi(t, 2, TEST_INT(hash_map_get(&map, keys[1])));
    test_eqi(t, 3, TEST_INT(hash_map_get(&map, keys[2])));

    hash_map_destroy(&map);
}

static void test_hash_map_null_value(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);

    hash_map_set(&map, 1, NULL);
    test_eqi(t, 1, map.count);

    // overwriting a NULL value doesn't count twice
    hash_map_set(&map, 1, TEST_VALUE(5));
    test_eqi(t, 1, map.count);

    hash_map_set(&map, 1, NULL);
    hash_map_set(&map, 2, TEST_VALUE(2));

    // NULL entries are still iterated
    int iterated = 0;
    for (struct hash_map_entry* entry = hash_map_next(&map, NULL); entry; entry = hash_map_next(&map, entry)) {
        iterated += 1;
    }
    test_eqi(t, 2, iterated);

    hash_map_delete(&map, 1);
    test_eqi(t, 1, map.count);
    test_eqi(t, 2, TEST_INT(hash_map_get(&map, 2)));

    hash_map_destroy(&map);
}

static void test_hash_map_grow(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);
    int initial_capacity = map.capacity;

    for (int key = 1; key <= 1000; key += 1) {
        test_eqi(t, true, hash_map_set(&map, key, TEST_VALUE(key * 3)));
    }

    test_eqi(t, 1000, map.count);
    test_eqi(t, true, map.capacity > initial_capacity);
    // load factor stays under 3/4
    test_eqi(t, true, map.count * 4 < map.capacity * 3);
    // but doesn't waste memory
    test_eqi(t, true, map.count * 8 >= map.capacity * 3);

    for (int key = 1; key <= 1000; key += 1) {
        test_eqi(t, key * 3, TEST_INT(hash_map_get(&map, key)));
    }

    test_eqi(t, 0, TEST_INT(hash_map_get(&map, 1001)));

    hash_map_destroy(&map);
}

static void test_hash_map_iterate(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);

    test_eqi(t, 0, (int)(intptr_t)hash_map_next(&map, NULL));

    int seen[64] = {0};

    for (int key = 1; key < 64; key += 1) {
        hash_map_set(&map, key, TEST_VALUE(key));
    }

    for (int key = 1; key < 64; key += 2) {
        hash_map_delete(&map, key);
    }

    int iterated = 0;

    for (struct hash_map_entry* entry = hash_map_next(&map, NULL); entry; entry = hash_map_next(&map, entry)) {
        test_eqi(t, entry->key, TEST_INT(entry->value));
        test_eqi(t, 0, entry->key & 1);
        seen[entry->key] += 1;
        iterated += 1;
    }

    test_eqi(t, map.count, iterated);

    for (int key = 2; key < 64; key += 2) {
        test_eqi(t, 1, seen[key]);
    }

    hash_map_destroy(&map);
}

static void test_hash_map_destroyed(struct test_context* t) {
    hash_map_t map;
    hash_map_init(&map, 8);
    hash_map_set(&map, 1, TEST_VALUE(1));
    hash_map_destroy(&map);

    // operations on a destroyed map don't crash
    test_eqi(t, 0, TEST_INT(hash_map_get(&map, 1)));
    hash_map_delete(&map, 1);
    test_eqi(t, 0, (int)(intptr_t)hash_map_next(&map, NULL));

    // setting reinitializes the map
    test_eqi(t, true, hash_map_set(&map, 2, TEST_VALUE(2)));
    test_eqi(t, 2, TEST_INT(hash_map_get(&map, 2)));

    hash_map_destroy(&map);
}

// compares against a simple array under random inserts and deletes
static void test_hash_map_random(struct test_context* t) {
    #define RANDOM_KEY_RANGE    256

    hash_map_t map;
    hash_map_init(&map, 8);

    int expected[RANDOM_KEY_RANGE] = {0};
    int expected_count = 0;
    uint32_t seed = 12345;

    for (int step = 0; step < 4000; step += 1) {
        seed = seed * 1103515245 + 12345;
        // keys spaced apart and negative to exercise the hash
        int index = (seed >> 16) % RANDOM_KEY_RANGE;
        int key = (index - RANDOM_KEY_RANGE / 2) * 1024 + 1;
        bool should_delete = (seed >> 8) & 1;

        if (should_delete) {
            if (expected[index]) {
                expected_count -= 1;
            }
            expected[index] = 0;
            hash_map_delete(&map, key);
        } else {
            if (!expected[index]) {
                expected_count += 1;
            }
            expected[index] = step + 1;
            hash_map_set(&map, key, TEST_VALUE(step + 1));
        }

        test_eqi(t, expected_count, map.count);
    }

    for (int index = 0; index < RANDOM_KEY_RANGE; index += 1) {
        int key = (index - RANDOM_KEY_RANGE / 2) * 1024 + 1;
        test_eqi(t, expected[index], TEST_INT(hash_map_get(&map, key)));
    }

    hash_map_destroy(&map);

    #undef RANDOM_KEY_RANGE
}

void test_hash_map(struct test_context* t) {
    test_hash_map_basic(t);
    test_hash_map_init_capacity(t);
    test_hash_map_collision(t);
    test_hash_map_delete_keeps_home(t);
    test_hash_map_wrap_around(t);
    test_hash_map_null_value(t);
    test_hash_map_grow(t);
    test_hash_map_iterate(t);
    test_hash_map_destroyed(t);
    test_hash_map_random(t);
}
