#ifndef __RESOURCE_TMESH_CACHE_H__
#define __RESOURCE_TMESH_CACHE_H__

#include "../render/tmesh.h"

struct tmesh* mesh_cache_load(const char* filename);
void mesh_cache_release(struct tmesh* mesh);

#endif