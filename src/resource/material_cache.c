#include "material_cache.h"

#include <libdragon.h>
#include <t3d/t3d.h>
#include <malloc.h>
#include "resource_cache.h"
#include "sprite_cache.h"
#include "../util/cleanup.h"

struct resource_cache material_resource_cache;

void* material_cache_entry_load(const char* filename) {
    material_pair_t* result = malloc(sizeof(material_pair_t));
    
    FILE* material_file = asset_fopen(filename, NULL);
    material_pair_load(result, material_file);
    material_debug(&result->apply, filename);
    result->is_embedded = false;
    fclose(material_file);

    return result;
}

void material_cache_entry_free(void* material) {
    material_pair_release(material);
    free(material);
}

void material_cache_swap(void* a, void* b) {
    material_pair_t* new_resource = (material_pair_t*)a;
    material_pair_t* existing_resource = (material_pair_t*)b;

    material_pair_t tmp = *new_resource;
    *new_resource = *existing_resource;
    *existing_resource = tmp;
}

void material_cache_init() {
    resource_cache_register(
        &material_resource_cache, 
        material_cache_entry_load,
        material_cache_entry_free,
        material_cache_swap,
        ".mat"
    );
}

material_pair_t* material_cache_load(const char* filename) {
    return resource_cache_use(&material_resource_cache, filename);
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