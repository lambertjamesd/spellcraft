#include "animation_cache.h"

#include "resource_cache.h"

static struct resource_cache animation_resource_cache;

struct animation_set* animation_cache_load(const char* filename) {
    resource_cache_maybe_init(&animation_resource_cache, (resource_cache_free_callback)annotation_clip_set_free);
    struct resource_cache_entry* entry = resource_cache_use(&animation_resource_cache, filename);

    if (!entry->resource) {
        resource_cache_set_resource(&animation_resource_cache, entry, animation_set_load(filename));
    }

    return entry->resource;
}
void animation_cache_release(struct animation_set* animations) {
    resource_cache_free(&animation_resource_cache, animations);
}

void animation_cache_destroy() {
    resource_cache_destroy(&animation_resource_cache);
}