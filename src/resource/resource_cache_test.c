#include "../test/framework_test.h"

#include "resource_cache.h"

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

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

// simulates the main loop queuing a sweep and the RSP catching up
static void sweep_and_wait() {
    resource_cache_update();
    rspq_wait();
}

// the resource returned by the next call to test_load
static void* next_resource;
static int load_count;

static void* test_load(const char* filename) {
    load_count += 1;
    return next_resource;
}

static void reset_caches(resource_cache_free_callback callback, resource_cache_free_callback other_callback) {
    // flush anything left over from a previous failed test
    resource_cache_flush();

    memset(&cache, 0, sizeof(resource_cache_t));
    memset(&other_cache, 0, sizeof(resource_cache_t));
    resource_cache_register(&cache, test_load, callback, NULL, NULL);
    resource_cache_register(&other_cache, test_load, other_callback, NULL, NULL);

    free_log_count = 0;
    other_free_log_count = 0;
    load_count = 0;
    next_resource = NULL;
}

// uses name, loading it as resource if it isn't already loaded
static void* load(resource_cache_t* target, const char* name, void* resource) {
    next_resource = resource;
    return resource_cache_use(target, name);
}

static resource_cache_entry_t* find(resource_cache_t* target, const char* name) {
    return resource_cache_find_entry(target, name, false);
}

static void test_resource_cache_reference_counting(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    // entries are allocated on first use
    test_eqi(t, 0, (int)cache.entries);
    test_eqi(t, 0, (int)find(&cache, "a"));

    test_eqi(t, (int)RESOURCE_A, (int)load(&cache, "a", RESOURCE_A));
    resource_cache_entry_t* entry = find(&cache, "a");
    test_eqi(t, (int)RESOURCE_A, (int)entry->resource);
    test_eqi(t, 1, entry->reference_count);
    test_eqi(t, 1, load_count);

    // already loaded so the loader shouldn't be called again
    test_eqi(t, (int)RESOURCE_A, (int)load(&cache, "a", RESOURCE_B));
    entry = find(&cache, "a");
    test_eqi(t, 2, entry->reference_count);
    test_eqi(t, 1, cache.entry_count);
    test_eqi(t, 1, load_count);

    resource_cache_free(&cache, RESOURCE_A);
    sweep_and_wait();
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    // removal is deferred until the RSP catches up
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    sweep_and_wait();
    test_eqi(t, 1, free_log_count);
    test_eqi(t, (int)RESOURCE_A, (int)free_log[0]);
    test_eqi(t, 0, cache.entry_count);

    // loading again after removal should start from scratch
    test_eqi(t, (int)RESOURCE_B, (int)load(&cache, "a", RESOURCE_B));
    test_eqi(t, 2, load_count);
    test_eqi(t, 1, find(&cache, "a")->reference_count);
    resource_cache_free(&cache, RESOURCE_B);
    sweep_and_wait();
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
    test_eqi(t, (int)RESOURCE_A, (int)load(&cache, "a", RESOURCE_B));
    test_eqi(t, 1, find(&cache, "a")->reference_count);
    test_eqi(t, 1, load_count);

    sweep_and_wait();
    test_eqi(t, 0, free_log_count);
    test_eqi(t, 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    sweep_and_wait();
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
    sweep_and_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_A));
    test_eqi(t, 0, free_count_for(RESOURCE_COLLISION));

    // the colliding entry should still be found by both name and resource
    test_eqi(t, (int)RESOURCE_COLLISION, (int)load(&cache, "b", RESOURCE_B));
    test_eqi(t, 2, find(&cache, "b")->reference_count);

    resource_cache_free(&cache, RESOURCE_COLLISION);
    resource_cache_free(&cache, RESOURCE_COLLISION);
    sweep_and_wait();
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
        int random_index = ((((uint32_t)i + 1) * PRIME_NUMBER) % LARGE_NUMBER_COUNT) + 1;
        load(&cache, name, (void*)(random_index * 16));
        test_eqi(t, 1, find(&cache, name)->reference_count);
    }

    test_eqi(t, LARGE_NUMBER_COUNT, load_count);

    test_eqi(t, LARGE_NUMBER_COUNT, cache.entry_count);

    // every name should map to the same resource after resizing
    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 1) {
        char name[10];
        sprintf(name, "%i", i);
        int random_index = ((((uint32_t)i + 1) * PRIME_NUMBER) % LARGE_NUMBER_COUNT) + 1;
        void* resource = load(&cache, name, NULL);
        test_eqi(t, random_index * 16, (int)resource);
        test_eqi(t, 2, find(&cache, name)->reference_count);
        resource_cache_free(&cache, resource);
    }

    // release many entries from the same cache in a single frame. The cache
    // should only be scheduled for a sweep once
    for (int i = 0; i < LARGE_NUMBER_COUNT; i += 2) {
        resource_cache_free(&cache, (void*)((i + 1) * 16));
    }

    sweep_and_wait();
    test_eqi(t, LARGE_NUMBER_COUNT / 2, free_log_count);
    test_eqi(t, LARGE_NUMBER_COUNT / 2, cache.entry_count);

    for (int i = 1; i < LARGE_NUMBER_COUNT; i += 2) {
        resource_cache_free(&cache, (void*)((i + 1) * 16));
    }

    sweep_and_wait();
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
    sweep_and_wait();
    test_eqi(t, 2, cache.entry_count);

    test_eqi(t, 0x801d5a58, (int)load(&cache, "rom:/meshes/vehicles/bike.anim", NULL));
    resource_cache_free(&cache, (void*)0x801d5a58);

    resource_cache_free(&cache, (void*)0x801d5a58);
    resource_cache_free(&cache, (void*)0x801da518);
    sweep_and_wait();
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
    sweep_and_wait();

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

    sweep_and_wait();
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
    sweep_and_wait();
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

    // a destroyed cache can be used again without registering it again
    test_eqi(t, (int)record_free, (int)cache.free_callback);
    test_eqi(t, (int)test_load, (int)cache.load);
    load(&cache, "a", RESOURCE_A);
    resource_cache_free(&cache, RESOURCE_A);
    sweep_and_wait();
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
    sweep_and_wait();
    test_eqi(t, 0, free_count_for(RESOURCE_B));

    // even if the sweep for A was dropped, the sweep for B should also remove A
    resource_cache_free(&cache, RESOURCE_B);
    sweep_and_wait();
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

static void test_resource_cache_release_after_sweep_queued(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    load(&cache, "a", RESOURCE_A);
    load(&cache, "b", RESOURCE_B);

    // without an update no sweep is queued
    resource_cache_free(&cache, RESOURCE_A);
    rspq_wait();
    test_eqi(t, 0, free_log_count);

    resource_cache_update();
    // released after the sweep was queued so the RSP may still be using it
    resource_cache_free(&cache, RESOURCE_B);
    // only one sweep should be pending at a time
    resource_cache_update();
    rspq_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_A));
    test_eqi(t, 0, free_count_for(RESOURCE_B));
    test_eqi(t, 1, cache.entry_count);

    // the next update should queue another sweep for the remaining entry
    sweep_and_wait();
    test_eqi(t, 1, free_count_for(RESOURCE_B));
    test_eqi(t, 0, cache.entry_count);

    // nothing left to sweep
    sweep_and_wait();
    test_eqi(t, 2, free_log_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_lazy_init(struct test_context* t) {
    reset_caches(record_free, record_other_free);

    // nothing to free or reload before the first use
    test_eqi(t, 0, (int)cache.entries);
    test_eqi(t, false, resource_cache_reload_single(&cache, "a"));
    resource_cache_destroy(&cache);

    load(&cache, "a", RESOURCE_A);
    resource_cache_entry_t* entries_before = cache.entries;
    test_neqi(t, 0, (int)entries_before);

    // using another resource should not reset the cache
    load(&cache, "b", RESOURCE_B);
    test_eqi(t, (int)entries_before, (int)cache.entries);
    test_eqi(t, (int)record_free, (int)cache.free_callback);

    resource_cache_free(&cache, RESOURCE_A);
    resource_cache_free(&cache, RESOURCE_B);
    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static resource_cache_t* recursive_cache_target;

// loads a resource from the same cache, simulating a resource 
// that depends on another resource of the same type
static void* recursive_load(const char* filename) {
    if (strcmp(filename, "parent") != 0) {
        return (void*)(uintptr_t)(atoi(filename) * 16);
    }

    char name[10];

    // enough entries to resize the cache while the parent entry is being loaded
    for (int i = 0; i < LARGE_NUMBER_COUNT / 4; i += 1) {
        sprintf(name, "%d", i + 10);
        resource_cache_use(recursive_cache_target, name);
    }

    return RESOURCE_A;
}

static void test_resource_cache_recursive_load(struct test_context* t) {
    reset_caches(record_free, record_other_free);
    resource_cache_register(&cache, recursive_load, record_free, NULL, NULL);
    recursive_cache_target = &cache;

    test_eqi(t, (int)RESOURCE_A, (int)resource_cache_use(&cache, "parent"));

    resource_cache_entry_t* entry = find(&cache, "parent");
    test_eqi(t, (int)RESOURCE_A, (int)entry->resource);
    test_eqi(t, 1, entry->reference_count);
    test_eqi(t, LARGE_NUMBER_COUNT / 4 + 1, cache.entry_count);

    resource_cache_free(&cache, RESOURCE_A);
    for (int i = 0; i < LARGE_NUMBER_COUNT / 4; i += 1) {
        char name[10];
        sprintf(name, "%d", i + 10);
        resource_cache_free(&cache, find(&cache, name)->resource);
    }

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

/////////////////////////
// hot reload swapping
/////////////////////////

#define SWAP_EXT    ".swaptest"

struct test_resource {
    int size;
    int value;
};

typedef struct test_resource test_resource_t;

#define MAX_SWAP_FREE_LOG   16

static int swap_free_values[MAX_SWAP_FREE_LOG];
static int swap_free_count;
static int swap_count;

static test_resource_t* test_resource_new(int size, int value) {
    test_resource_t* result = malloc(sizeof(test_resource_t));
    result->size = size;
    result->value = value;
    return result;
}

static void test_resource_free(void* resource) {
    test_resource_t* test_resource = resource;

    if (swap_free_count < MAX_SWAP_FREE_LOG) {
        swap_free_values[swap_free_count] = test_resource->value;
    }
    swap_free_count += 1;

    free(resource);
}

// rejects the swap if the size changes, like a sprite changing width
static void test_resource_swap(void* a, void* b) {
    test_resource_t* new_resource = a;
    test_resource_t* existing_resource = b;

    swap_count += 1;

    if (new_resource->size != existing_resource->size) {
        return;
    }

    test_resource_t tmp = *new_resource;
    *new_resource = *existing_resource;
    *existing_resource = tmp;
}

static void reset_swap_cache() {
    reset_caches(record_free, record_other_free);
    resource_cache_register(&cache, test_load, test_resource_free, test_resource_swap, SWAP_EXT);
    swap_free_count = 0;
    swap_count = 0;
}

static void test_resource_cache_swap_in_place(struct test_context* t) {
    reset_swap_cache();

    test_resource_t* existing = load(&cache, "rom:/a" SWAP_EXT, test_resource_new(4, 1));
    // a second reference to the same resource
    load(&cache, "rom:/a" SWAP_EXT, NULL);

    next_resource = test_resource_new(4, 2);
    test_eqi(t, true, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 2, load_count);
    test_eqi(t, 1, swap_count);

    // the existing pointer should now hold the new contents
    test_eqi(t, 2, existing->value);
    // the old contents should be freed right away
    test_eqi(t, 1, swap_free_count);
    test_eqi(t, 1, swap_free_values[0]);

    // the cache should still know about the existing pointer
    resource_cache_entry_t* entry = find(&cache, "rom:/a" SWAP_EXT);
    test_eqi(t, (int)existing, (int)entry->resource);
    test_eqi(t, 2, entry->reference_count);
    test_eqi(t, 1, cache.entry_count);
    test_eqi(t, (int)existing, (int)load(&cache, "rom:/a" SWAP_EXT, NULL));

    // releasing should free the swapped resource exactly once
    resource_cache_free(&cache, existing);
    resource_cache_free(&cache, existing);
    resource_cache_free(&cache, existing);
    sweep_and_wait();
    test_eqi(t, 2, swap_free_count);
    test_eqi(t, 2, swap_free_values[1]);
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_swap_rejected(struct test_context* t) {
    reset_swap_cache();

    test_resource_t* existing = load(&cache, "rom:/a" SWAP_EXT, test_resource_new(4, 1));

    // the size changed so the swap should be rejected
    next_resource = test_resource_new(8, 2);
    test_eqi(t, true, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 1, swap_count);

    test_eqi(t, 4, existing->size);
    test_eqi(t, 1, existing->value);
    // the rejected resource should still be freed
    test_eqi(t, 1, swap_free_count);
    test_eqi(t, 2, swap_free_values[0]);

    resource_cache_free(&cache, existing);
    sweep_and_wait();
    test_eqi(t, 2, swap_free_count);
    test_eqi(t, 1, swap_free_values[1]);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_swap_ignored(struct test_context* t) {
    reset_swap_cache();

    // nothing loaded yet
    next_resource = NULL;
    test_eqi(t, false, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 0, load_count);

    test_resource_t* existing = load(&cache, "rom:/a" SWAP_EXT, test_resource_new(4, 1));

    // files that aren't loaded should not be loaded by a reload
    test_eqi(t, false, resource_cache_reload("rom:/b" SWAP_EXT));
    // unknown extensions and files without an extension
    test_eqi(t, false, resource_cache_reload("rom:/a.unknown"));
    test_eqi(t, false, resource_cache_reload("rom:/a"));
    test_eqi(t, false, resource_cache_reload("rom:/a" SWAP_EXT "/b"));
    test_eqi(t, 1, load_count);
    test_eqi(t, 0, swap_count);

    // a failed load should leave the resource alone
    next_resource = NULL;
    test_eqi(t, false, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 0, swap_count);
    test_eqi(t, 1, existing->value);

    resource_cache_free(&cache, existing);
    sweep_and_wait();
    test_eqi(t, 1, swap_free_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_swap_pending_release(struct test_context* t) {
    reset_swap_cache();

    test_resource_t* existing = load(&cache, "rom:/a" SWAP_EXT, test_resource_new(4, 1));
    // released but waiting on a sweep
    resource_cache_free(&cache, existing);

    next_resource = test_resource_new(4, 2);
    test_eqi(t, true, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 2, existing->value);
    test_eqi(t, 1, swap_free_count);

    // the sweep should still remove the swapped entry
    sweep_and_wait();
    test_eqi(t, 2, swap_free_count);
    test_eqi(t, 2, swap_free_values[1]);
    test_eqi(t, 0, cache.entry_count);

    resource_cache_destroy(&cache);
    resource_cache_destroy(&other_cache);
}

static void test_resource_cache_swap_register(struct test_context* t) {
    reset_swap_cache();

    // registering the same cache again should reuse its hot reload slot
    for (int i = 0; i < 32; i += 1) {
        resource_cache_register(&cache, test_load, test_resource_free, test_resource_swap, SWAP_EXT);
    }

    test_resource_t* existing = load(&cache, "rom:/a" SWAP_EXT, test_resource_new(4, 1));

    // registering without a swap disables hot reloading
    resource_cache_register(&cache, test_load, test_resource_free, NULL, SWAP_EXT);
    next_resource = NULL;
    test_eqi(t, false, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 1, load_count);

    // and it can be enabled again
    resource_cache_register(&cache, test_load, test_resource_free, test_resource_swap, SWAP_EXT);
    next_resource = test_resource_new(4, 2);
    test_eqi(t, true, resource_cache_reload("rom:/a" SWAP_EXT));
    test_eqi(t, 2, existing->value);

    resource_cache_free(&cache, existing);
    sweep_and_wait();
    test_eqi(t, 2, swap_free_count);

    resource_cache_register(&cache, test_load, record_free, NULL, NULL);
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
    test_resource_cache_release_after_sweep_queued(t);
    test_resource_cache_lazy_init(t);
    test_resource_cache_recursive_load(t);
    test_resource_cache_swap_in_place(t);
    test_resource_cache_swap_rejected(t);
    test_resource_cache_swap_ignored(t);
    test_resource_cache_swap_pending_release(t);
    test_resource_cache_swap_register(t);
}
