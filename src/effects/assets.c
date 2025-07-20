#include "assets.h"

#include "../resource/tmesh_cache.h"

static struct effect_assets assets;

void effect_assets_init() {
    // tmesh_cache_release() never called
    assets.drop_shadow = tmesh_cache_load("rom:/meshes/effects/drop-shadow.tmesh");

    // tmesh_cache_release() never called
    assets.hit_effect = tmesh_cache_load("rom:/meshes/effects/hit_effect.tmesh");
}

struct effect_assets* effect_assets_get() {
    return &assets;
}