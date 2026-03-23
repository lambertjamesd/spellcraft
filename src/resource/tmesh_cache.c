#include "tmesh_cache.h"

#include "resource_cache.h"

struct resource_cache tmesh_resource_cache;

struct tmesh* tmesh_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&tmesh_resource_cache, filename);

    if (!entry->resource) {
        struct tmesh* result = malloc(sizeof(struct tmesh));
        
        FILE* meshFile = asset_fopen(filename, NULL);
        tmesh_load(result, meshFile);
        fclose(meshFile);

        resource_cache_set_resource(&tmesh_resource_cache, entry, result);
    }

    return entry->resource;
}

void tmesh_cache_release(struct tmesh* mesh) {
    if (resource_cache_free(&tmesh_resource_cache, mesh)) {
        tmesh_release(mesh);
        free(mesh);
    }
}

void tmesh_cache_destroy() {
    resource_cache_destroy(&tmesh_resource_cache);
}