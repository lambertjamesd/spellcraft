#ifndef __RESOURCE_MESH_CACHE_H__
#define __RESOURCE_MESH_CACHE_H__

#include "../render/mesh.h"

struct mesh* mesh_cache_load(const char* filename);
void mesh_cache_release(struct mesh* mesh);

#endif