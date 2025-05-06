#include "assets.h"

#include "../resource/tmesh_cache.h"

static struct object_assets object_assets;

void object_assets_init() {
    // tmesh_cache_release() never called
    object_assets.mana_gem_mesh = tmesh_cache_load("rom:/meshes/objects/mana_gem.tmesh");
}

struct object_assets* object_assets_get() {
    return &object_assets;
}