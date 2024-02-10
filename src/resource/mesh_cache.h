#ifndef __RESOURCE_MESH_CACHE_H__
#define __RESOURCE_MESH_CACHE_H__

#include "../render/mesh.h"

void mesh_load(struct mesh* into, FILE* meshFile);
void mesh_release(struct mesh* mesh);

struct mesh* mesh_cache_load(const char* filename);
void mesh_cache_release(struct mesh* mesh);

#endif