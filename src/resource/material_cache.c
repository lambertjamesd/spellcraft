#include "material_cache.h"

#include <libdragon.h>
#include <t3d/t3d.h>
#include <malloc.h>
#include "resource_cache.h"
#include "sprite_cache.h"
#include "../util/cleanup.h"

struct resource_cache material_resource_cache;

void material_cache_entry_free(void* material) {
    material_pair_release(material);
    free(material);
}

material_pair_t* material_cache_load(const char* filename) {
    resource_cache_maybe_init(&material_resource_cache, material_cache_entry_free);
    struct resource_cache_entry* entry = resource_cache_use(&material_resource_cache, filename);

    if (!entry->resource) {
        material_pair_t* result = malloc(sizeof(material_pair_t));
        
        FILE* material_file = asset_fopen(filename, NULL);
        material_pair_load(result, material_file);
        material_debug(&result->apply, filename);
        result->is_embedded = false;
        fclose(material_file);

        resource_cache_set_resource(&material_resource_cache, entry, result);
    }

    return entry->resource;
}

void material_cache_release_direct(material_pair_t* material) {
    resource_cache_free(&material_resource_cache, material);
}

void material_cache_release(material_pair_t* material) {
    // the resource cache already waits for the RSP before freeing
    resource_cache_free(&material_resource_cache, material);
}

material_pair_t* material_cache_load_from_file(FILE* file) {
    uint8_t material_name_length;
    fread(&material_name_length, 1, 1, file);

    if (!material_name_length) {
        return NULL;
    }

    char material_name[material_name_length + 1];
    fread(&material_name[0], 1, material_name_length, file);
    material_name[material_name_length] = '\0';

    return material_cache_load(material_name);
}

material_pair_t* material_cache_load_linked_or_embedded(FILE* file) {
    material_pair_t* result = material_cache_load_from_file(file);

    if (result) {
        return result;
    }

    result = malloc(sizeof(material_pair_t));
    material_load(&result->apply, file);
    material_init(&result->revert);
    result->is_embedded = true;

    return result;
}

void material_cache_release_linked_or_embedded(material_pair_t* material) {
    if (material->is_embedded) {
        material_release(&material->apply);
        free(material);
    } else {
        material_cache_release_direct(material);
    }
}

void material_cache_destroy() {
    resource_cache_destroy(&material_resource_cache);
}