#include "wav_cache.h"

#include "resource_cache.h"

static struct resource_cache wav_resource_cache;

static void* wav_cache_entry_load(const char* filename) {
    return wav64_load(filename, NULL);
}

// wav64 streams from ROM so it isn't hot reloaded
void wav_cache_init() {
    resource_cache_register(
        &wav_resource_cache,
        wav_cache_entry_load,
        (resource_cache_free_callback)wav64_close,
        NULL,
        NULL
    );
}

wav64_t* wav_cache_load(const char* filename) {
    return resource_cache_use(&wav_resource_cache, filename);
}

void wav_cache_release(wav64_t* wav) {
    resource_cache_free(&wav_resource_cache, wav);
}

void wav_cache_destroy() {
    resource_cache_destroy(&wav_resource_cache);
}