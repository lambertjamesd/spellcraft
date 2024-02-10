#ifndef __SCENE_WORLD_LOADER_H__
#define __SCENE_WORLD_LOADER_H__

#include "world.h"

struct world* world_load(const char* filename);
void world_release(struct world* world);

#endif