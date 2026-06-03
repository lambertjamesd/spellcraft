#include "material_cache.h"

#include <libdragon.h>
#include <t3d/t3d.h>
#include <malloc.h>
#include "resource_cache.h"
#include "sprite_cache.h"

struct resource_cache material_resource_cache;

material_pair_t* material_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&material_resource_cache, filename);

    if (!entry->resource) {
        material_pair_t* result = malloc(sizeof(material_pair_t));
        
        FILE* material_file = asset_fopen(filename, NULL);
        material_pair_load(result, material_file);
        material_debug(&result->apply, filename);
        fclose(material_file);

        resource_cache_set_resource(&material_resource_cache, entry, result);
    }

    return entry->resource;
}

void material_cache_release(material_pair_t* material) {
    if (resource_cache_free(&material_resource_cache, material)) {
        material_pair_release(material);
        free(material);
    }
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

void material_cache_destroy() {
    resource_cache_destroy(&material_resource_cache);
}