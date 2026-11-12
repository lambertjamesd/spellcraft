#include "sprite_cache.h"

#include "resource_cache.h"
#include "../util/cleanup.h"

struct resource_cache sprite_resource_cache;

sprite_t* sprite_cache_load(const char* filename) {
    resource_cache_maybe_init(&sprite_resource_cache, (resource_cache_free_callback)sprite_free);
    struct resource_cache_entry* entry = resource_cache_use(&sprite_resource_cache, filename);

    if (entry->resource == NULL) {
        resource_cache_set_resource(&sprite_resource_cache, entry, sprite_load(filename));
    }

    return entry->resource;
}

// TODO cleanup
void sprite_cache_release_direct(sprite_t* sprite) {
    resource_cache_free(&sprite_resource_cache, sprite);
}

void sprite_cache_release(sprite_t* sprite) {
    resource_cache_free(&sprite_resource_cache, sprite);
}

void sprite_cache_destroy() {
    resource_cache_destroy(&sprite_resource_cache);
}