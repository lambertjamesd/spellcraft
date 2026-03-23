#include "hash_map.h"

#include "../test/framework_test.h"

#include <stddef.h>

void test_hash_map(struct test_context* t) {
    hash_map_t map;

    hash_map_init(&map, 8);

    test_eqi(t, 0, (int)hash_map_get(&map, 1));
    hash_map_set(&map, 1, (void*)1);
    test_eqi(t, 1, (int)hash_map_get(&map, 1));

    // creates a collision
    hash_map_set(&map, map.capacity + 1, (void*)65);
    test_eqi(t, 1, (int)hash_map_get(&map, 1));
    test_eqi(t, 65, (int)hash_map_get(&map, map.capacity + 1));

    // delete entry where collided entry wants to be
    hash_map_delete(&map, 1);
    
    // 65 should be moved back to expected entry
    test_eqi(t, 0, (int)hash_map_get(&map, 1));
    test_eqi(t, 65, (int)hash_map_get(&map, map.capacity + 1));
}