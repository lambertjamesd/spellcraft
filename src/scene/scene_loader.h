#ifndef __SCENE_SCENE_LOADER_H__
#define __SCENE_SCENE_LOADER_H__

#include "scene.h"

struct scene* scene_load(const char* filename);
void scene_release(struct scene* scene);

struct entity_definition* scene_get_entity(unsigned index);

#endif