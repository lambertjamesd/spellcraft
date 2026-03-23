#include "../test/framework_test.h"

#include "resource_cache.h"

#define RESOURCE_A              (void*)16
#define RESOURCE_COLLISION      (void*)(16 + 16 * 32)

#define PRIME_NUMBER      12303871

#define LARGE_NUMBER_COUNT  200

void test_resource_cache(struct test_context* t) {
    resource_cache_t cache;
    memset(&cache, 0, sizeof(resource_cache_t));

    resource_cache_entry_t* entry = resource_cache_use(&cache, "a");
    
    test_eqi(t, 0, (int)entry->resource);
    resource_cache_set_resource(&cache, entry, RESOURCE_A);
    test_eqi(t, 1, entry->reference_count);

    entry = resource_cache_use(&cache, "a");
    test_eqi(t, (int)RESOURCE_A, (int)entry->resource);
    test_eqi(t, 2, entry->reference_count);

    bool should_free = resource_cache_free(&cache, RESOURCE_A);
    test_eqi(t, false, should_free);

    should_free = resource_cache_free(&cache, RESOURCE_A);
    test_eqi(t, true, should_free);

    resource_cache_destroy(&cache);

    entry = resource_cache_use(&cache, "a");
    resource_cache_set_resource(&cache, entry, RESOURCE_A);
    entry = resource_cache_use(&cache, "b");
    resource_cache_set_resource(&cache, entry, RESOURCE_COLLISION);
    
    should_free = resource_cache_free(&cache, RESOURCE_A);
    test_eqi(t, true, should_free);
    should_free = resource_cache_free(&cache, RESOURCE_COLLISION);
    test_eqi(t, true, should_free);
    
    resource_cache_destroy(&cache);

    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 1) {
        char name[10];
        sprintf(name, "%i", i);
        entry = resource_cache_use(&cache, name);

        int random_index = ((((uint32_t)i + 1) * PRIME_NUMBER) % LARGE_NUMBER_COUNT) + 1;

        test_eqi(t, 1, entry->reference_count);
        resource_cache_set_resource(&cache, entry, (void*)(random_index * 16));
    }
    
    for (int i = 0; i < 200; i += 1) {
        should_free = resource_cache_free(&cache, (void*)((i + 1) * 16));
        test_eqi(t, true, should_free);
    }
    
    resource_cache_destroy(&cache);

    entry = resource_cache_use(&cache, "rom:/meshes/characters/scrapper_kid.anim");
    resource_cache_set_resource(&cache, entry, (void*)0x80179d10);
    entry = resource_cache_use(&cache, "rom:/meshes/vehicles/bike.anim");
    resource_cache_set_resource(&cache, entry, (void*)0x801d5a58);
    entry = resource_cache_use(&cache, "rom:/meshes/characters/NPC_scrapbot1.anim");
    resource_cache_set_resource(&cache, entry, (void*)0x801da518);
    entry = resource_cache_use(&cache, "rom:/meshes/doors/garage_door.anim");
    resource_cache_set_resource(&cache, entry, (void*)0x801dda18);
    
    should_free = resource_cache_free(&cache, (void*)0x80179d10);
    test_eqi(t, true, should_free);
    should_free = resource_cache_free(&cache, (void*)0x801dda18);
    test_eqi(t, true, should_free);
    should_free = resource_cache_free(&cache, (void*)0x801d5a58);
    test_eqi(t, true, should_free);
    should_free = resource_cache_free(&cache, (void*)0x801da518);
    test_eqi(t, true, should_free);
    
    resource_cache_destroy(&cache);
}