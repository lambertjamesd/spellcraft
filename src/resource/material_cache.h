#ifndef __RESOURCE_MATERIAL_CACHE_H__
#define __RESOURCE_MATERIAL_CACHE_H__

#include "../render/material.h"
#include <stdio.h>

// load a material or reuse the existing one if already loaded
// callers of this function must call material_cache_release
// when they are done with the material
material_pair_t* material_cache_load(const char* filename);
void material_cache_release(material_pair_t* material);

material_pair_t* material_cache_load_from_file(FILE* file);

material_pair_t* material_cache_load_linked_or_embedded(FILE* file);
void material_cache_release_linked_or_embedded(material_pair_t*);

void material_cache_destroy();

#endif