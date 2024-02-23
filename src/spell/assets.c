#include "assets.h"

#include "../resource/mesh_cache.h"

static struct spell_assets assets;

void spell_assets_init() {
    assets.projectile_mesh = mesh_cache_load("rom:/meshes/spell/projectile.mesh");
}

struct spell_assets* spell_assets_get() {
    return &assets;
}