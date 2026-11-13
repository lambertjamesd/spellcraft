#include "../test/framework_test.h"

#include "resource_cache.h"

#define RESOURCE_A              (void*)16
#define RESOURCE_B              (void*)32
#define RESOURCE_COLLISION      (void*)(16 + 16 * 32)

#define PRIME_NUMBER      12303871

#define LARGE_NUMBER_COUNT  200

#define MAX_FREE_LOG        256

// caches are static so a failed test that leaves a sweep
// pending never points the sweep at a dead stack frame
static resource_cache_t cache;
static resource_cache_t other_cache;

static void* free_log[MAX_FREE_LOG];
static int free_log_count;

static void* other_free_log[MAX_FREE_LOG];
static int other_free_log_count;

static void record_free(void* resource) {
    if (free_log_count < MAX_FREE_LOG) {
        free_log[free_log_count] = resource;
    }
    free_log_count += 1;
}

static void record_other_free(void* resource) {
    if (other_free_log_count < MAX_FREE_LOG) {
        other_free_log[other_free_log_count] = resource;
    }
    other_free_log_count += 1;
}

static int free_count_for(void* resource) {
    int result = 0;
    for (int i = 0; i < free_log_count && i < MAX_FREE_LOG; i += 1) {
        if (free_log[i] == resource) {
            result += 1;
        }
    }
    return result;
}

static void reset_caches(resource_cache_free_callback callback, resource_cache_free_callback other_callback) {
    // flush anything left over from a previous failed test
    resource_cache_flush();

    memset(&cache, 0, sizeof(resource_cache_t));
    memset(&other_cache, 0, sizeof(resource_cache_t));
    resource_cache_init(&cache, callback);
    resource_cache_init(&other_cache, other_callback);

    free_log_count = 0;
    other_free_log_count = 0;
}

static resource_cache_entry_t* load(resource_cache_t* target, const char* name, void* resource) {
    resource_cache_entry_t* entry = resource_cache_use(target, name);

    if (!entry->resource) {
        resource_cache_set_resource(target, entry, resource);
    }

    return entry;
}

static void test_resource_cache_reference_counting(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    resource_cache_entry_t* entry = resource_cache_use(&cache, "a");
    test_eqi(t, 0, (int)entry->resource);
    test_eqi(t, 1, entry->reference_count);
    resource_cache_set_resource(&cache, entry, RESOURCE_A);

    entry = resource_cache_use(&cache, "a");
    test_eqi(t, (int)RESOURCE_A, (int)entry->resource);
    test_eqi(t, 2, entry->reference_count);
    test_eqi(t, 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    rspq_wait();
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    // removal is deferred until the RSP catches up
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    rspq_wait();
    test_eqi(t, 1, free_log_count);
    test_eqi(t, (int)RESOURCE_A, (int)free_log[0]);
    test_eqi(t, 0, cache.entry_count);

    // loading again after removal should start from scratch
    entry = resource_cache_use(&cache, "a");
    test_eqi(t, 0, (int)entry->resource);
    test_eqi(t, 1, entry->reference_count);
    resource_cache_set_resource(&cache, entry, RESOURCE_B);
    resource_cache_free(&cache, RESOURCE_B);
    rspq_wait();
    test_eqi(t, 2, free_log_count);
    test_eqi(t, (int)RESOURCE_B, (int)free_log[1]);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_revive_before_sweep(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    resource_cache_free(&cache, RESOURCE_A);

    // using the resource again before the sweep should keep it alive
    resource_cache_entry_t* entry = resource_cache_use(&cache, "a");
    test_eqi(t, (int)RESOURCE_A, (int)entry->resource);
    test_eqi(t, 1, entry->reference_count);

    rspq_wait();
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    rspq_wait();
    test_eqi(t, 1, free_log_count);
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_hash_collision(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    load(&cache, "b", RESOURCE_COLLISION);

    resource_cache_free(&cache, RESOURCE_A);
    rspq_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_A));
    test_eqi(t, 0, free_count_for(RESOURCE_COLLISION));

    // the colliding entry should still be found by both name and resource
    resource_cache_entry_t* entry = resource_cache_use(&cache, "b");
    test_eqi(t, (int)RESOURCE_COLLISION, (int)entry->resource);
    test_eqi(t, 2, entry->reference_count);

    resource_cache_free(&cache, RESOURCE_COLLISION);
    resource_cache_free(&cache, RESOURCE_COLLISION);
    rspq_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_COLLISION));
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_many_entries(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    // enough entries to force several resizes
    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 1) {
        char name[10];
        sprintf(name, "%i", i);
        resource_cache_entry_t* entry = resource_cache_use(&cache, name);

        int random_index = ((((uint32_t)i + 1) * PRIME_NUMBER) % LARGE_NUMBER_COUNT) + 1;

        test_eqi(t, 1, entry->reference_count);
        test_eqi(t, 0, (int)entry->resource);
        resource_cache_set_resource(&cache, entry, (void*)(random_index * 16));
    }

    test_eqi(t, LARGE_NUMBER_COUNT, cache.entry_count);

    // every name should map to the same resource after resizing
    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 1) {
        char name[10];
        sprintf(name, "%i", i);
        resource_cache_entry_t* entry = resource_cache_use(&cache, name);
        int random_index = ((((uint32_t)i + 1) * PRIME_NUMBER) % LARGE_NUMBER_COUNT) + 1;
        test_eqi(t, random_index * 16, (int)entry->resource);
        test_eqi(t, 2, entry->reference_count);
        resource_cache_free(&cache, entry->resource);
    }

    // release many entries from the same cache in a single frame. The cache
    // should only be scheduled for a sweep once
    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 2) {
        resource_cache_free(&cache, (void*)((i + 1) * 16));
    }

    rspq_wait();
    test_eqi(t, LARGE_NUMBER_COUNT / 2, free_log_count);
    test_eqi(t, LARGE_NUMBER_COUNT / 2, cache.entry_count);

    for (int i = 1; i < LARGE_NUMBER_COUNT; i += 2) {
        resource_cache_free(&cache, (void*)((i + 1) * 16));
    }

    rspq_wait();
    test_eqi(t, LARGE_NUMBER_COUNT, free_log_count);
    test_eqi(t, 0, cache.entry_count);

    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 1) {
        test_eqi(t, 1, free_count_for((void*)((i + 1) * 16)));
    }

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_real_addresses(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "rom:/meshes/characters/scrapper_kid.anim", (void*)0x80179d10);
    load(&cache, "rom:/meshes/vehicles/bike.anim", (void*)0x801d5a58);
    load(&cache, "rom:/meshes/characters/NPC_scrapbot1.anim", (void*)0x801da518);
    load(&cache, "rom:/meshes/doors/garage_door.anim", (void*)0x801dda18);

    resource_cache_free(&cache, (void*)0x80179d10);
    resource_cache_free(&cache, (void*)0x801dda18);
    rspq_wait();
    test_eqi(t, 2, cache.entry_count);

    resource_cache_entry_t* entry = resource_cache_use(&cache, "rom:/meshes/vehicles/bike.anim");
    test_eqi(t, 0x801d5a58, (int)entry->resource);
    resource_cache_free(&cache, (void*)0x801d5a58);

    resource_cache_free(&cache, (void*)0x801d5a58);
    resource_cache_free(&cache, (void*)0x801da518);
    rspq_wait();
    test_eqi(t, 4, free_log_count);
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_multiple_caches(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    load(&other_cache, "a", RESOURCE_B);

    resource_cache_free(&cache, RESOURCE_A);
    resource_cache_free(&other_cache, RESOURCE_B);
    rspq_wait();

    test_eqi(t, 1, free_log_count);
    test_eqi(t, (int)RESOURCE_A, (int)free_log[0]);
    test_eqi(t, 1, other_free_log_count);
    test_eqi(t, (int)RESOURCE_B, (int)other_free_log[0]);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static bool dependency_released_immediately;

// simulates a tmesh releasing its material when it is freed
static void release_dependency(void* resource) {
    record_free(resource);
    resource_cache_free(&other_cache, RESOURCE_B);
    // a release triggered by the sweep happens immediately
    // since the RSP is already known to be done with it
    dependency_released_immediately = other_cache.entry_count == 0 && other_free_log_count == 1;
}

static void test_resource_cache_cascading_free(struct test_context* t) {
    reset_caches(release_dependency, record_other_free);
    dependency_released_immediately = false;

    load(&other_cache, "material", RESOURCE_B);
    load(&cache, "mesh", RESOURCE_A);

    resource_cache_free(&cache, RESOURCE_A);
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 0, other_free_log_count);

    rspq_wait();
    test_eqi(t, 1, free_log_count);
    test_eqi(t, true, dependency_released_immediately);
    test_eqi(t, 1, other_free_log_count);
    test_eqi(t, (int)RESOURCE_B, (int)other_free_log[0]);
    test_eqi(t, 0, cache.entry_count);
    test_eqi(t, 0, other_cache.entry_count);

    // frees after the sweep should be deferred again
    load(&other_cache, "material", RESOURCE_B);
    resource_cache_free(&other_cache, RESOURCE_B);
    test_eqi(t, 1, other_free_log_count);
    rspq_wait();
    test_eqi(t, 2, other_free_log_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_destroy_flushes(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    load(&cache, "b", RESOURCE_B);
    resource_cache_free(&cache, RESOURCE_A);
    resource_cache_free(&cache, RESOURCE_B);

    // destroy should process the pending removal instead of asserting
    resource_cache_destroy(&cache);
    test_eqi(t, 2, free_log_count);
    test_eqi(t, 0, (int)cache.entries);

    // destroying twice is harmless
    resource_cache_destroy(&cache);

    // a destroyed cache can be initialized again
    resource_cache_maybe_init(&cache, record_free);
    load(&cache, "a", RESOURCE_A);
    resource_cache_free(&cache, RESOURCE_A);
    rspq_wait();
    test_eqi(t, 3, free_log_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void release_resource_a(void* data) {
    resource_cache_free(&cache, RESOURCE_A);
}

static void test_resource_cache_release_in_deferred_call(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    load(&cache, "b", RESOURCE_B);

    // libdragon can drop a deferred call that is queued from inside another 
    // deferred call. Releasing a resource from cleanup code does exactly that
    rspq_call_deferred(release_resource_a, NULL);
    rspq_wait();
    test_eqi(t, 0, free_count_for(RESOURCE_B));

    // even if the sweep for A was dropped, the sweep for B should also remove A
    resource_cache_free(&cache, RESOURCE_B);
    rspq_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_A));
    test_eqi(t, 1, free_count_for(RESOURCE_B));
    test_eqi(t, 0, cache.entry_count);

    // resource_cache_flush should also recover a dropped sweep
    load(&cache, "a", RESOURCE_A);
    rspq_call_deferred(release_resource_a, NULL);
    resource_cache_flush();
    test_eqi(t, 2, free_count_for(RESOURCE_A));
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_maybe_init(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    resource_cache_entry_t* entries_before = cache.entries;
    resource_cache_maybe_init(&cache, record_other_free);
    // already initialized, should not reset the cache or change the callback
    test_eqi(t, (int)entries_before, (int)cache.entries);
    test_eqi(t, (int)record_free, (int)cache.free_callback);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

void test_resource_cache(struct test_context* t) {
    test_resource_cache_reference_counting(t);
    test_resource_cache_revive_before_sweep(t);
    test_resource_cache_hash_collision(t);
    test_resource_cache_many_entries(t);
    test_resource_cache_real_addresses(t);
    test_resource_cache_multiple_caches(t);
    test_resource_cache_cascading_free(t);
    test_resource_cache_destroy_flushes(t);
    test_resource_cache_release_in_deferred_call(t);
    test_resource_cache_maybe_init(t);
}
