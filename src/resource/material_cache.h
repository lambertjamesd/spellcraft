#ifndef __RESOURCE_MATERIAL_CACHE_H__
#define __RESOURCE_MATERIAL_CACHE_H__

#include "../render/material.h"

struct material* material_cache_load(const char* filename);
void material_cache_release(struct material* material);

#endif