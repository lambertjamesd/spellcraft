#include "wav_cache.h"

#include "resource_cache.h"

static struct resource_cache wav_resource_cache;

wav64_t* wav_cache_load(const char* filename) {
    resource_cache_maybe_init(&wav_resource_cache, (resource_cache_free_callback)wav64_close);
    struct resource_cache_entry* entry = resource_cache_use(&wav_resource_cache, filename);

    if (entry->resource == NULL) {
        resource_cache_set_resource(&wav_resource_cache, entry, wav64_load(filename, NULL));
    }

    return entry->resource;
}

void wav_cache_release(wav64_t* wav) {
    resource_cache_free(&wav_resource_cache, wav);
}

void wav_cache_destroy() {
    resource_cache_destroy(&wav_resource_cache);
}