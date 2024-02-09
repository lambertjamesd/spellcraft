#ifndef __RESOURCE_MATERIAL_CACHE_H__
#define __RESOURCE_MATERIAL_CACHE_H__

#include "../render/material.h"

// used to directly load a material
// materials loaded this way must be 
// released with material_release
void material_load(struct material* into, FILE* material_file);
void material_release(struct material* material);

// load a material or reuse the existing one if already loaded
// callers of this function must call material_cache_release
// when they are done with the material
struct material* material_cache_load(const char* filename);
void material_cache_release(struct material* material);

#endif