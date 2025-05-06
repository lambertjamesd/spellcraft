#ifndef __OBJECTS_ASSETS_H__
#define __OBJECTS_ASSETS_H__

#include "../render/tmesh.h"
#include "../render/animation_clip.h"

struct object_assets {
    struct tmesh* mana_gem_mesh;
};

void object_assets_init();
struct object_assets* object_assets_get();

#endif